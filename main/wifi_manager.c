#include "wifi_manager.h"
#include "display_ui.h"
#include "webserver.h"
#include "wikiquote.h"
#include "sleep_manager.h"
#include "battery.h"
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_mac.h"
#include "esp_sntp.h"
#include "nvs_flash.h"
#include "nvs.h"

static const char *TAG = "WIFI_MANAGER";

#define WIFI_NVS_NAMESPACE "wifi_config"
#define WIFI_SSID_KEY "ssid"
#define WIFI_PASS_KEY "password"
#define WIFI_QUOTE_COUNT_KEY "quote_count"
#define AP_SSID_PREFIX "WMQuote_"
#define MAX_RETRY 3
#define MAX_RETRY_CYCLES 10
#define RETRY_DELAY_MS 60000  // 1 minute

static int retry_count = 0;
static int retry_cycle = 0;
static bool provisioning_mode = false;
static bool display_updated = false;
static TaskHandle_t connection_task_handle = NULL;
static TimerHandle_t retry_timer = NULL;
static uint32_t quote_count = 0;

// Forward declarations
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data);
static void start_provisioning_mode(void);
static esp_err_t load_credentials(char* ssid, char* password);
static void start_sta_mode(const char* ssid, const char* password);
static void initialize_sntp(void);
static void get_formatted_time(char* buffer, size_t buffer_size);
static esp_err_t load_quote_count(void);

esp_err_t wifi_manager_init(void) {
    ESP_LOGI(TAG, "Initializing WiFi manager...");

    // Initialize TCP/IP network interface
    ESP_ERROR_CHECK(esp_netif_init());

    // Create default event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Create default WiFi station and AP network interfaces
    esp_netif_create_default_wifi_sta();
    esp_netif_create_default_wifi_ap();

    // Initialize WiFi with default config
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Register event handlers
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                               &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                               &wifi_event_handler, NULL));

    // Load quote counter from NVS
    load_quote_count();

    ESP_LOGI(TAG, "WiFi manager initialized");
    return ESP_OK;
}

esp_err_t wifi_manager_start(bool silent) {
    ESP_LOGI(TAG, "Starting WiFi manager...");

    char ssid[33] = {0};
    char password[65] = {0};

    // Try to load credentials from NVS
    esp_err_t err = load_credentials(ssid, password);

    if (err == ESP_OK && strlen(ssid) > 0) {
        // Credentials found, try to connect
        ESP_LOGI(TAG, "Found saved credentials for SSID: %s", ssid);
        if (!silent) {
            display_connecting(ssid);
        } else {
            ESP_LOGI(TAG, "Silent reconnection, skipping connection message");
        }
        start_sta_mode(ssid, password);
    } else {
        // No credentials, start provisioning mode
        ESP_LOGI(TAG, "No credentials found, starting provisioning mode");
        start_provisioning_mode();
    }

    return ESP_OK;
}

esp_err_t wifi_manager_save_credentials(const char* ssid, const char* password) {
    ESP_LOGI(TAG, "Saving WiFi credentials to NVS...");

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(WIFI_NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS handle: %s", esp_err_to_name(err));
        return err;
    }

    // Save SSID
    err = nvs_set_str(nvs_handle, WIFI_SSID_KEY, ssid);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error saving SSID: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    // Save password
    err = nvs_set_str(nvs_handle, WIFI_PASS_KEY, password);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error saving password: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    // Commit changes
    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error committing NVS: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "WiFi credentials saved successfully");
    }

    nvs_close(nvs_handle);
    return err;
}

static esp_err_t load_credentials(char* ssid, char* password) {
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(WIFI_NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "No saved credentials found");
        return err;
    }

    size_t ssid_len = 33;
    size_t pass_len = 65;

    err = nvs_get_str(nvs_handle, WIFI_SSID_KEY, ssid, &ssid_len);
    if (err == ESP_OK) {
        err = nvs_get_str(nvs_handle, WIFI_PASS_KEY, password, &pass_len);
    }

    nvs_close(nvs_handle);
    return err;
}

static void start_sta_mode(const char* ssid, const char* password) {
    ESP_LOGI(TAG, "Starting WiFi in station mode...");

    wifi_config_t wifi_config = {0};
    strlcpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    strlcpy((char*)wifi_config.sta.password, password, sizeof(wifi_config.sta.password));

    // Increase beacon timeout threshold to reduce warnings
    wifi_config.sta.listen_interval = 3;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    // Set WiFi power save mode to reduce beacon timeout warnings
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_MIN_MODEM));

    ESP_ERROR_CHECK(esp_wifi_start());

    retry_count = 0;
    provisioning_mode = false;
    display_updated = false;  // Reset flag for new connection
}

static void start_provisioning_mode(void) {
    ESP_LOGI(TAG, "Starting provisioning mode (AP)...");

    // Get MAC address to create unique SSID
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);

    // Create SSID: WMQuote_XX (last 2 hex digits of MAC)
    char ap_ssid[32];
    snprintf(ap_ssid, sizeof(ap_ssid), "%s%02X", AP_SSID_PREFIX, mac[5]);

    ESP_LOGI(TAG, "AP SSID: %s", ap_ssid);

    wifi_config_t ap_config = {
        .ap = {
            .ssid_len = 0,  // Let ESP-IDF calculate from string
            .max_connection = 1,
            .authmode = WIFI_AUTH_OPEN,
            .channel = 1,
        },
    };

    // Copy SSID to config
    strlcpy((char*)ap_config.ap.ssid, ap_ssid, sizeof(ap_config.ap.ssid));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    // Start web server
    webserver_start();

    // Update display
    display_provisioning_mode(ap_ssid);

    provisioning_mode = true;
    display_updated = false;  // Reset flag for next connection attempt
}

static void initialize_sntp(void) {
    ESP_LOGI(TAG, "Initializing SNTP...");

    // Set timezone to Europe/Rome (CET-1CEST with DST)
    setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
    tzset();

    // Initialize SNTP
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_init();

    ESP_LOGI(TAG, "SNTP initialized, waiting for time sync...");

    // Wait for time to be set (max 10 seconds)
    int retry = 0;
    const int retry_count = 10;
    while (esp_sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    if (retry < retry_count) {
        ESP_LOGI(TAG, "Time synchronized successfully");
    } else {
        ESP_LOGW(TAG, "Time sync timeout, using default time");
    }
}

static void get_formatted_time(char* buffer, size_t buffer_size) {
    time_t now;
    struct tm timeinfo;

    time(&now);
    localtime_r(&now, &timeinfo);

    // Format: "Last update: DD/MM/YYYY HH:MM"
    strftime(buffer, buffer_size, "Last update: %d/%m/%Y %H:%M", &timeinfo);
}

// Timer callback to retry WiFi connection after delay
static void retry_timer_callback(TimerHandle_t xTimer) {
    ESP_LOGI(TAG, "Retry timer expired, attempting to reconnect (cycle %d/%d)...",
             retry_cycle + 1, MAX_RETRY_CYCLES);
    retry_count = 0;  // Reset retry count for new cycle
    retry_cycle++;
    esp_wifi_connect();
}

// Task to handle connection setup (SNTP, quote fetching, display update)
// Runs in separate task with larger stack to avoid overflow
static void connection_setup_task(void* param) {
    ESP_LOGI(TAG, "Connection setup task started");

    // Initialize SNTP and get current time
    initialize_sntp();

    // Initialize and read battery
    esp_err_t batt_err = battery_init();
    float battery_percent = -1.0;
    if (batt_err == ESP_OK) {
        battery_percent = battery_read_percentage();
        if (battery_percent >= 0) {
            ESP_LOGI(TAG, "Battery percentage: %.1f%%", battery_percent);
        } else {
            ESP_LOGW(TAG, "Failed to read battery percentage");
        }
    } else {
        ESP_LOGW(TAG, "Battery init failed: %s", esp_err_to_name(batt_err));
    }

    // Initialize wikiquote
    wikiquote_init();

    // Calculate random sleep duration BEFORE displaying (needed for next update time)
    uint32_t min_sleep_minutes = 10;
    uint32_t max_sleep_minutes = 60;
    uint32_t random_minutes = min_sleep_minutes + (esp_random() % (max_sleep_minutes - min_sleep_minutes + 1));
    uint32_t sleep_seconds = random_minutes * 60;

    // Get a random quote with author
    char quote[512];
    char author[128];
    esp_err_t err = wikiquote_get_random_quote_with_author(quote, sizeof(quote),
                                                            author, sizeof(author));

    // Increment quote counter
    wifi_manager_increment_quote_count();

    // Format datetime string with quote counter and next update time
    char datetime_str[192];
    char time_part[64];
    get_formatted_time(time_part, sizeof(time_part));

    // Calculate next update time
    time_t now;
    time(&now);
    time_t next_update = now + sleep_seconds;
    struct tm next_update_tm;
    localtime_r(&next_update, &next_update_tm);
    char next_update_str[32];
    strftime(next_update_str, sizeof(next_update_str), "%H:%M", &next_update_tm);

    // Format datetime string with battery percentage
    if (battery_percent >= 0) {
        snprintf(datetime_str, sizeof(datetime_str), "%s - quotes: %lu - next: %s - batt: %.0f%%",
                 time_part, (unsigned long)wifi_manager_get_quote_count(), next_update_str, battery_percent);
    } else {
        snprintf(datetime_str, sizeof(datetime_str), "%s - quotes: %lu - next: %s - batt: --%%",
                 time_part, (unsigned long)wifi_manager_get_quote_count(), next_update_str);
    }

    // Update display with quote, author and time
    if (err == ESP_OK) {
        display_connected_mode(quote, author, datetime_str);
    } else {
        // Fallback if quote fetch fails
        display_connected_mode("La semplicità è l'ultima sofisticazione.",
                              "Leonardo da Vinci", datetime_str);
    }

    display_updated = true;

    ESP_LOGI(TAG, "Connection setup task completed");

    // Wait a bit to ensure display is fully powered off
    vTaskDelay(pdMS_TO_TICKS(2000));

    // Enter deep sleep
    ESP_LOGI(TAG, "Entering deep sleep for %lu minutes (%lu seconds)...",
             (unsigned long)random_minutes, (unsigned long)sleep_seconds);
    sleep_manager_enter_deep_sleep(sleep_seconds);

    // This line will never be reached as device enters deep sleep
    connection_task_handle = NULL;
    vTaskDelete(NULL);
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                ESP_LOGI(TAG, "WiFi station started, connecting...");
                esp_wifi_connect();
                break;

            case WIFI_EVENT_STA_DISCONNECTED:
                if (!provisioning_mode) {
                    if (retry_count < MAX_RETRY) {
                        ESP_LOGI(TAG, "Connection failed, retrying... (%d/%d)",
                                retry_count + 1, MAX_RETRY);
                        esp_wifi_connect();
                        retry_count++;
                    } else {
                        // Reached max retry attempts for this cycle
                        if (retry_cycle < MAX_RETRY_CYCLES) {
                            ESP_LOGW(TAG, "Failed to connect after %d attempts (cycle %d/%d), waiting 1 minute before retry...",
                                    MAX_RETRY, retry_cycle + 1, MAX_RETRY_CYCLES);

                            // Create one-shot timer for retry delay
                            if (retry_timer != NULL) {
                                xTimerDelete(retry_timer, 0);
                            }
                            retry_timer = xTimerCreate("retry_timer",
                                                       pdMS_TO_TICKS(RETRY_DELAY_MS),
                                                       pdFALSE,  // One-shot timer
                                                       NULL,
                                                       retry_timer_callback);
                            if (retry_timer != NULL) {
                                xTimerStart(retry_timer, 0);
                            }
                        } else {
                            ESP_LOGW(TAG, "Failed to connect after %d retry cycles, switching to provisioning mode",
                                    MAX_RETRY_CYCLES);
                            // Stop STA mode first
                            esp_wifi_stop();
                            // Start provisioning mode
                            start_provisioning_mode();
                        }
                    }
                }
                break;

            case WIFI_EVENT_AP_STACONNECTED: {
                wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
                ESP_LOGI(TAG, "Station "MACSTR" connected to AP, AID=%d",
                        MAC2STR(event->mac), event->aid);
                break;
            }

            case WIFI_EVENT_AP_STADISCONNECTED: {
                wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
                ESP_LOGI(TAG, "Station "MACSTR" disconnected from AP, AID=%d",
                        MAC2STR(event->mac), event->aid);
                break;
            }

            default:
                break;
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP address: " IPSTR, IP2STR(&event->ip_info.ip));

        // Only update display once to prevent flashing on DHCP renewals
        if (!display_updated && connection_task_handle == NULL) {
            // Create task with large stack for HTTPS, JSON parsing, and display
            xTaskCreate(connection_setup_task,
                       "conn_setup",
                       12288,  // 12KB stack for HTTPS and JSON
                       NULL,
                       5,
                       &connection_task_handle);
        }

        retry_count = 0;
        retry_cycle = 0;

        // Clean up retry timer if it exists
        if (retry_timer != NULL) {
            xTimerDelete(retry_timer, 0);
            retry_timer = NULL;
        }
    }
}

static esp_err_t load_quote_count(void) {
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(WIFI_NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "No saved quote count found, starting from 0");
        quote_count = 0;
        return ESP_OK;
    }

    err = nvs_get_u32(nvs_handle, WIFI_QUOTE_COUNT_KEY, &quote_count);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to read quote count, starting from 0");
        quote_count = 0;
    } else {
        ESP_LOGI(TAG, "Loaded quote count: %lu", (unsigned long)quote_count);
    }

    nvs_close(nvs_handle);
    return ESP_OK;
}

uint32_t wifi_manager_get_quote_count(void) {
    return quote_count;
}

esp_err_t wifi_manager_increment_quote_count(void) {
    quote_count++;
    ESP_LOGI(TAG, "Incrementing quote count to %lu", (unsigned long)quote_count);

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(WIFI_NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS handle for quote count: %s", esp_err_to_name(err));
        return err;
    }

    err = nvs_set_u32(nvs_handle, WIFI_QUOTE_COUNT_KEY, quote_count);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error saving quote count: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error committing quote count: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "Quote count saved successfully");
    }

    nvs_close(nvs_handle);
    return err;
}

esp_err_t wifi_manager_delete_credentials(void) {
    ESP_LOGI(TAG, "Deleting WiFi credentials from NVS...");

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(WIFI_NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS handle for credential deletion: %s", esp_err_to_name(err));
        return err;
    }

    // Erase SSID
    err = nvs_erase_key(nvs_handle, WIFI_SSID_KEY);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGE(TAG, "Error erasing SSID: %s", esp_err_to_name(err));
    }

    // Erase password
    err = nvs_erase_key(nvs_handle, WIFI_PASS_KEY);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGE(TAG, "Error erasing password: %s", esp_err_to_name(err));
    }

    // Commit changes
    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error committing NVS changes: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "WiFi credentials deleted successfully");
    }

    nvs_close(nvs_handle);
    return err;
}
