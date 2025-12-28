#pragma once

#include <esp_err.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize WiFi manager
 * Sets up WiFi subsystem, event loop, and network interface
 *
 * @return ESP_OK on success
 */
esp_err_t wifi_manager_init(void);

/**
 * Start WiFi manager
 * Checks NVS for credentials and either connects or starts provisioning mode
 *
 * @param silent If true, skip displaying connection message (for wake from sleep)
 * @return ESP_OK on success
 */
esp_err_t wifi_manager_start(bool silent);

/**
 * Save WiFi credentials to NVS
 * Stores SSID and password for persistent configuration
 *
 * @param ssid WiFi SSID (max 32 characters)
 * @param password WiFi password (max 64 characters)
 * @return ESP_OK on success
 */
esp_err_t wifi_manager_save_credentials(const char* ssid, const char* password);

/**
 * Get current quote counter value
 * Retrieves the total number of quotes displayed from NVS
 *
 * @return Current quote count
 */
uint32_t wifi_manager_get_quote_count(void);

/**
 * Increment quote counter
 * Increments and saves the quote counter to NVS
 *
 * @return ESP_OK on success
 */
esp_err_t wifi_manager_increment_quote_count(void);

/**
 * Delete WiFi credentials from NVS
 * Erases saved SSID and password, forcing provisioning mode on next boot
 *
 * @return ESP_OK on success
 */
esp_err_t wifi_manager_delete_credentials(void);

#ifdef __cplusplus
}
#endif
