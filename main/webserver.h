#pragma once

#include <esp_err.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Start the HTTP configuration web server
 * Serves the WiFi configuration page and handles credential submission
 *
 * @return ESP_OK on success
 */
esp_err_t webserver_start(void);

/**
 * Stop the HTTP configuration web server
 *
 * @return ESP_OK on success
 */
esp_err_t webserver_stop(void);

#ifdef __cplusplus
}
#endif
