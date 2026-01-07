#pragma once

#include <esp_err.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize battery voltage monitoring
 * Configures ADC1 Channel 0 (GPIO 36) with calibration
 *
 * NOTE: Must be called before battery_read_percentage()
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t battery_init(void);

/**
 * Read battery percentage
 * Powers on EPD voltage divider, reads ADC, calculates percentage
 *
 * Uses averaged readings (64 samples) for stability
 * Accounts for 2:1 voltage divider on hardware
 *
 * @return Battery percentage (0-100), or -1.0 on error
 */
float battery_read_percentage(void);

/**
 * Read raw battery voltage
 * Useful for debugging and logging
 *
 * @return Battery voltage in volts, or -1.0 on error
 */
float battery_read_voltage(void);

#ifdef __cplusplus
}
#endif
