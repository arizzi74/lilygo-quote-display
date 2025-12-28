#include "webserver.h"
#include "wifi_manager.h"
#include "config_page.h"
#include <string.h>
#include <ctype.h>
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_system.h"

static const char *TAG = "WEBSERVER";

static httpd_handle_t server = NULL;

// URL decode helper function
static void url_decode(char *dst, const char *src) {
    char a, b;
    while (*src) {
        if ((*src == '%') &&
            ((a = src[1]) && (b = src[2])) &&
            (isxdigit(a) && isxdigit(b))) {
            if (a >= 'a')
                a -= 'a'-'A';
            if (a >= 'A')
                a -= ('A' - 10);
            else
                a -= '0';
            if (b >= 'a')
                b -= 'a'-'A';
            if (b >= 'A')
                b -= ('A' - 10);
            else
                b -= '0';
            *dst++ = 16*a+b;
            src+=3;
        } else if (*src == '+') {
            *dst++ = ' ';
            src++;
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}

// Handler for GET / - serves the configuration page
static esp_err_t config_page_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "Serving configuration page");
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, config_html, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// Handler for POST /save - receives WiFi credentials
static esp_err_t save_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "Received WiFi configuration");

    char buf[256];
    int ret, remaining = req->content_len;

    // Read POST body
    if (remaining >= sizeof(buf)) {
        ESP_LOGE(TAG, "Content too long");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Content too long");
        return ESP_FAIL;
    }

    ret = httpd_req_recv(req, buf, remaining);
    if (ret <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }
    buf[ret] = '\0';

    ESP_LOGI(TAG, "Received data: %s", buf);

    // Parse form data
    char ssid[33] = {0};
    char password[65] = {0};
    char ssid_encoded[100] = {0};
    char pass_encoded[100] = {0};

    // Extract SSID
    esp_err_t err = httpd_query_key_value(buf, "ssid", ssid_encoded, sizeof(ssid_encoded));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "SSID not found in form data");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "SSID required");
        return ESP_FAIL;
    }
    url_decode(ssid, ssid_encoded);

    // Extract password (optional)
    err = httpd_query_key_value(buf, "password", pass_encoded, sizeof(pass_encoded));
    if (err == ESP_OK) {
        url_decode(password, pass_encoded);
    } else {
        password[0] = '\0';  // Empty password for open networks
    }

    // Validate SSID
    if (strlen(ssid) == 0 || strlen(ssid) > 32) {
        ESP_LOGE(TAG, "Invalid SSID length: %d", strlen(ssid));
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid SSID");
        return ESP_FAIL;
    }

    // Validate password
    if (strlen(password) > 64) {
        ESP_LOGE(TAG, "Invalid password length: %d", strlen(password));
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid password");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Parsed credentials - SSID: %s, Password: %s", ssid,
             strlen(password) > 0 ? "****" : "(none)");

    // Save credentials
    err = wifi_manager_save_credentials(ssid, password);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save credentials");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to save");
        return ESP_FAIL;
    }

    // Send success response
    const char* resp =
        "<!DOCTYPE html>"
        "<html>"
        "<head>"
        "<meta charset='UTF-8'>"
        "<style>"
        "body{font-family:Arial;text-align:center;padding:50px;}"
        "h2{color:#4CAF50;}"
        "</style>"
        "</head>"
        "<body>"
        "<h2>✓ Configuration Saved!</h2>"
        "<p>Device is restarting...</p>"
        "<p>Please reconnect to your WiFi network.</p>"
        "</body>"
        "</html>";

    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);

    // Restart after a short delay to allow response to be sent
    ESP_LOGI(TAG, "Restarting in 2 seconds...");
    vTaskDelay(pdMS_TO_TICKS(2000));
    esp_restart();

    return ESP_OK;
}

esp_err_t webserver_start(void) {
    ESP_LOGI(TAG, "Starting web server...");

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;
    config.max_uri_handlers = 8;
    config.stack_size = 8192;

    esp_err_t err = httpd_start(&server, &config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error starting server: %s", esp_err_to_name(err));
        return err;
    }

    // Register GET / handler
    httpd_uri_t uri_get = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = config_page_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &uri_get);

    // Register POST /save handler
    httpd_uri_t uri_post = {
        .uri = "/save",
        .method = HTTP_POST,
        .handler = save_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &uri_post);

    ESP_LOGI(TAG, "Web server started successfully");
    return ESP_OK;
}

esp_err_t webserver_stop(void) {
    if (server != NULL) {
        ESP_LOGI(TAG, "Stopping web server...");
        httpd_stop(server);
        server = NULL;
    }
    return ESP_OK;
}
