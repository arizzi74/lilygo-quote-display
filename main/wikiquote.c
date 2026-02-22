#include "wikiquote.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_tls.h"
#include "esp_crt_bundle.h"
#include "cJSON.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>

static const char *TAG = "WIKIQUOTE";

#define QUOTABLE_API_URL "https://quotes-api-three.vercel.app/api/randomquote?language=it"
#define MAX_HTTP_OUTPUT_BUFFER 4096   // 4KB buffer (quotes are much smaller)
#define MAX_QUOTE_LENGTH 160          // Max allowed quote text length
#define MAX_FETCH_RETRIES 5           // Max retries when quote is too long

static char http_response_buffer[MAX_HTTP_OUTPUT_BUFFER];
static int http_response_len = 0;

// HTTP event handler
static esp_err_t http_event_handler(esp_http_client_event_t *evt) {
    switch(evt->event_id) {
        case HTTP_EVENT_ON_DATA:
            if (http_response_len + evt->data_len < MAX_HTTP_OUTPUT_BUFFER - 1) {  // Leave room for null terminator
                memcpy(http_response_buffer + http_response_len, evt->data, evt->data_len);
                http_response_len += evt->data_len;
            } else {
                ESP_LOGW(TAG, "Response buffer full, truncating data");
            }
            break;
        default:
            break;
    }
    return ESP_OK;
}

esp_err_t wikiquote_init(void) {
    ESP_LOGI(TAG, "Wikiquote client initialized");
    return ESP_OK;
}

esp_err_t wikiquote_get_random_quote(const char* author_name, char* quote_buffer, size_t buffer_size) {
    ESP_LOGI(TAG, "Fetching random quote from Quotable.io...");

    // Reset response buffer
    memset(http_response_buffer, 0, sizeof(http_response_buffer));
    http_response_len = 0;

    // Use Quotable.io API for random quotes (English)
    // For specific authors, use: https://api.quotable.io/random?author=author-slug
    const char *url = QUOTABLE_API_URL;

    ESP_LOGI(TAG, "API URL: %s", url);

    // Configure HTTP client with TLS certificate verification
    esp_http_client_config_t config = {
        .url = url,
        .event_handler = http_event_handler,
        .timeout_ms = 10000,         // 10 second timeout
        .buffer_size = 2048,         // 2KB internal buffer (quotes are small)
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
        .crt_bundle_attach = esp_crt_bundle_attach,  // Use certificate bundle for verification
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        return ESP_FAIL;
    }

    // Perform HTTP GET request
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        int content_length = esp_http_client_get_content_length(client);
        ESP_LOGI(TAG, "HTTP GET Status = %d, expected_length = %d, received = %d",
                status_code, content_length, http_response_len);

        if (status_code == 200 && http_response_len > 0) {
            // Null-terminate response
            http_response_buffer[http_response_len] = '\0';

            // Debug: Print first 500 and last 100 chars of response
            ESP_LOGI(TAG, "Response preview (first 500): %.500s", http_response_buffer);
            if (http_response_len > 100) {
                ESP_LOGI(TAG, "Response preview (last 100): %s",
                         http_response_buffer + http_response_len - 100);
            }

            // Parse JSON
            cJSON *root = cJSON_Parse(http_response_buffer);
            if (root == NULL) {
                const char *error_ptr = cJSON_GetErrorPtr();
                if (error_ptr != NULL) {
                    ESP_LOGE(TAG, "JSON parse error before: %.50s", error_ptr);
                }
                ESP_LOGE(TAG, "Failed to parse JSON (response length: %d)", http_response_len);
                esp_http_client_cleanup(client);
                return ESP_FAIL;
            }

            // Italian API JSON format: {"quote": "quote text", "author": "author name", "tags": "..."}
            cJSON *quote_field = cJSON_GetObjectItem(root, "quote");
            cJSON *author = cJSON_GetObjectItem(root, "author");

            if (quote_field && cJSON_IsString(quote_field)) {
                const char *quote_text = quote_field->valuestring;

                // Just return the quote text (author will be extracted separately)
                snprintf(quote_buffer, buffer_size, "%s", quote_text);

                ESP_LOGI(TAG, "Quote extracted: %.100s...", quote_buffer);
                cJSON_Delete(root);
                esp_http_client_cleanup(client);
                return ESP_OK;
            } else {
                ESP_LOGE(TAG, "Failed to find 'quote' field in JSON");
            }

            cJSON_Delete(root);
            err = ESP_FAIL;
        } else {
            ESP_LOGE(TAG, "HTTP request failed with status code: %d", status_code);
            err = ESP_FAIL;
        }
    } else {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);

    // Fallback quote if API fails
    if (err != ESP_OK) {
        snprintf(quote_buffer, buffer_size, "La semplicità è l'ultima sofisticazione.");
    }

    return err;
}

esp_err_t wikiquote_get_random_quote_with_author(char* quote_buffer, size_t quote_size,
                                                  char* author_buffer, size_t author_size) {
    ESP_LOGI(TAG, "Fetching random quote with author from Quotable.io...");

    const char *url = QUOTABLE_API_URL;

    esp_http_client_config_t config = {
        .url = url,
        .event_handler = http_event_handler,
        .timeout_ms = 10000,
        .buffer_size = 2048,
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };

    esp_err_t err = ESP_FAIL;

    for (int attempt = 1; attempt <= MAX_FETCH_RETRIES; attempt++) {
        ESP_LOGI(TAG, "Attempt %d/%d", attempt, MAX_FETCH_RETRIES);

        // Reset response buffer for each attempt
        memset(http_response_buffer, 0, sizeof(http_response_buffer));
        http_response_len = 0;

        esp_http_client_handle_t client = esp_http_client_init(&config);
        if (client == NULL) {
            ESP_LOGE(TAG, "Failed to initialize HTTP client");
            break;
        }

        err = esp_http_client_perform(client);

        if (err == ESP_OK) {
            int status_code = esp_http_client_get_status_code(client);
            ESP_LOGI(TAG, "HTTP GET Status = %d, received = %d", status_code, http_response_len);

            if (status_code == 200 && http_response_len > 0) {
                // Null-terminate response
                http_response_buffer[http_response_len] = '\0';

                // Parse JSON
                cJSON *root = cJSON_Parse(http_response_buffer);
                if (root == NULL) {
                    ESP_LOGE(TAG, "Failed to parse JSON");
                    esp_http_client_cleanup(client);
                    err = ESP_FAIL;
                    break;
                }

                // Extract both quote and author
                cJSON *quote_field = cJSON_GetObjectItem(root, "quote");
                cJSON *author = cJSON_GetObjectItem(root, "author");

                if (quote_field && cJSON_IsString(quote_field) && author && cJSON_IsString(author)) {
                    size_t quote_len = strlen(quote_field->valuestring);

                    if (quote_len > MAX_QUOTE_LENGTH) {
                        ESP_LOGW(TAG, "Quote too long (%d chars > %d), retrying...",
                                 (int)quote_len, MAX_QUOTE_LENGTH);
                        cJSON_Delete(root);
                        esp_http_client_cleanup(client);
                        err = ESP_FAIL;
                        continue;
                    }

                    snprintf(quote_buffer, quote_size, "%s", quote_field->valuestring);
                    snprintf(author_buffer, author_size, "%s", author->valuestring);

                    ESP_LOGI(TAG, "Quote (%d chars): %.100s...", (int)quote_len, quote_buffer);
                    ESP_LOGI(TAG, "Author: %s", author_buffer);

                    cJSON_Delete(root);
                    esp_http_client_cleanup(client);
                    return ESP_OK;
                } else {
                    ESP_LOGE(TAG, "Failed to find 'quote' or 'author' field in JSON");
                }

                cJSON_Delete(root);
                err = ESP_FAIL;
            } else {
                ESP_LOGE(TAG, "HTTP request failed with status code: %d", status_code);
                err = ESP_FAIL;
            }
        } else {
            ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
        }

        esp_http_client_cleanup(client);
        break;  // Only retry on quote-too-long; stop on HTTP/parse errors
    }

    // Fallback if all attempts fail
    snprintf(quote_buffer, quote_size, "La semplicità è l'ultima sofisticazione.");
    snprintf(author_buffer, author_size, "Leonardo da Vinci");

    return ESP_FAIL;
}
