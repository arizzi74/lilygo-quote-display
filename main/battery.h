#pragma once

#include <esp_err.h>
#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Battery reading data structure (stored in NVS)
 */
typedef struct {
    time_t timestamp;        // Unix timestamp of reading
    uint32_t adc_raw;        // Raw ADC value (0-4095)
    uint32_t voltage_mv;     // Voltage in millivolts (after calibration)
    float actual_voltage;    // Actual battery voltage (after divider compensation)
    float percentage;        // Battery percentage (0-100)
} battery_reading_t;

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
 * Automatically saves reading to NVS for debugging
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

/**
 * Get last battery reading from NVS
 * Useful for debugging when serial console not available
 *
 * @param reading Pointer to battery_reading_t structure to fill
 * @return ESP_OK if reading retrieved, ESP_ERR_NVS_NOT_FOUND if no reading stored
 */
esp_err_t battery_get_last_reading(battery_reading_t* reading);

/**
 * Print last battery reading from NVS to console
 * Useful for debugging after running on battery
 */
void battery_print_last_reading(void);

#ifdef __cplusplus
}
#endif
