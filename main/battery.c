#include "battery.h"
#include "epdiy.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <time.h>

static const char *TAG = "BATTERY";
#define BATTERY_NVS_NAMESPACE "battery_log"
#define BATTERY_READING_KEY "last_reading"

// Hardware configuration
#define BATT_PIN_GPIO 36
#define BATT_ADC_CHANNEL ADC1_CHANNEL_0    // GPIO 36 = ADC1_CH0
#define BATT_ADC_ATTEN ADC_ATTEN_DB_11     // 0-3.3V range
#define BATT_ADC_WIDTH ADC_WIDTH_BIT_12    // 12-bit resolution (0-4095)
#define BATT_SAMPLES 64                     // Number of samples to average

// Battery characteristics
#define BATT_VOLTAGE_DIVIDER 2.0           // Hardware voltage divider ratio
#define BATT_MIN_VOLTAGE 3.0               // Voltage at 0%
#define BATT_MAX_VOLTAGE 4.0               // Voltage at 100% (anything above = 100%)

// ADC calibration
static esp_adc_cal_characteristics_t adc_chars;
static bool initialized = false;

// Last reading cache
static battery_reading_t last_reading = {0};

esp_err_t battery_init(void) {
    ESP_LOGI(TAG, "Initializing battery voltage monitoring on GPIO %d...", BATT_PIN_GPIO);

    // Configure ADC1 width (resolution)
    esp_err_t err = adc1_config_width(BATT_ADC_WIDTH);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure ADC width: %s", esp_err_to_name(err));
        return err;
    }

    // Configure ADC1 channel with attenuation
    err = adc1_config_channel_atten(BATT_ADC_CHANNEL, BATT_ADC_ATTEN);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure ADC channel attenuation: %s", esp_err_to_name(err));
        return err;
    }

    // Characterize ADC for calibration (uses eFuse vref if available)
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(
        ADC_UNIT_1,
        BATT_ADC_ATTEN,
        BATT_ADC_WIDTH,
        1100,  // Default vref (will be overridden by eFuse if available)
        &adc_chars
    );

    // Log calibration method used
    if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
        ESP_LOGI(TAG, "ADC calibrated using eFuse Vref: %lu mV", (unsigned long)adc_chars.vref);
    } else if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
        ESP_LOGI(TAG, "ADC calibrated using eFuse Two Point");
    } else {
        ESP_LOGI(TAG, "ADC calibrated using Default Vref: 1100 mV");
    }

    initialized = true;
    ESP_LOGI(TAG, "Battery monitoring initialized successfully");
    return ESP_OK;
}

float battery_read_voltage(void) {
    if (!initialized) {
        ESP_LOGE(TAG, "Battery module not initialized! Call battery_init() first.");
        return -1.0;
    }

    // Power on EPD to enable voltage divider
    epd_poweron();
    vTaskDelay(pdMS_TO_TICKS(100));  // Increased stabilization time from 50ms to 100ms

    // Read multiple samples with delays and average
    // Adding delays between samples reduces noise
    uint32_t adc_sum = 0;
    for (int i = 0; i < BATT_SAMPLES; i++) {
        int raw = adc1_get_raw(BATT_ADC_CHANNEL);
        if (raw < 0) {
            ESP_LOGE(TAG, "ADC read error: %d", raw);
            epd_poweroff();
            return -1.0;
        }
        adc_sum += (uint32_t)raw;

        // Small delay between samples to allow ADC to settle
        if (i < BATT_SAMPLES - 1) {
            vTaskDelay(pdMS_TO_TICKS(2));
        }
    }
    uint32_t adc_average = adc_sum / BATT_SAMPLES;

    // Power off EPD
    epd_poweroff();

    // Convert ADC reading to voltage (in millivolts) using calibration
    uint32_t voltage_mv = esp_adc_cal_raw_to_voltage(adc_average, &adc_chars);

    // Account for voltage divider (2:1 ratio)
    float actual_voltage = ((float)voltage_mv / 1000.0) * BATT_VOLTAGE_DIVIDER;

    // Cache raw data for NVS logging
    last_reading.adc_raw = adc_average;
    last_reading.voltage_mv = voltage_mv;
    last_reading.actual_voltage = actual_voltage;

    ESP_LOGI(TAG, "Battery voltage: %.2f V (ADC avg: %lu, ADC mV: %lu)",
             actual_voltage, (unsigned long)adc_average, (unsigned long)voltage_mv);

    return actual_voltage;
}

float battery_read_percentage(void) {
    float voltage = battery_read_voltage();

    if (voltage < 0) {
        ESP_LOGE(TAG, "Failed to read battery voltage");
        return -1.0;
    }

    // Calculate percentage based on Li-ion discharge curve
    // 3.0V = 0% (safe cutoff), 3.7V = ~50% (nominal), 4.2V = 100%
    float percentage = ((voltage - BATT_MIN_VOLTAGE) / (BATT_MAX_VOLTAGE - BATT_MIN_VOLTAGE)) * 100.0;

    // Clamp to 0-100% range (handle charging spikes and deep discharge)
    if (percentage > 100.0) {
        percentage = 100.0;
    } else if (percentage < 0.0) {
        percentage = 0.0;
    }

    ESP_LOGI(TAG, "Battery percentage: %.1f%%", percentage);

    // Save complete reading to NVS for debugging
    last_reading.percentage = percentage;
    time(&last_reading.timestamp);

    // Save to NVS
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(BATTERY_NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err == ESP_OK) {
        err = nvs_set_blob(nvs_handle, BATTERY_READING_KEY, &last_reading, sizeof(battery_reading_t));
        if (err == ESP_OK) {
            nvs_commit(nvs_handle);
            ESP_LOGI(TAG, "Battery reading saved to NVS");
        } else {
            ESP_LOGW(TAG, "Failed to save battery reading to NVS: %s", esp_err_to_name(err));
        }
        nvs_close(nvs_handle);
    } else {
        ESP_LOGW(TAG, "Failed to open NVS for battery log: %s", esp_err_to_name(err));
    }

    return percentage;
}

esp_err_t battery_get_last_reading(battery_reading_t* reading) {
    if (reading == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(BATTERY_NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        return err;
    }

    size_t required_size = sizeof(battery_reading_t);
    err = nvs_get_blob(nvs_handle, BATTERY_READING_KEY, reading, &required_size);
    nvs_close(nvs_handle);

    return err;
}

void battery_print_last_reading(void) {
    battery_reading_t reading;
    esp_err_t err = battery_get_last_reading(&reading);

    if (err == ESP_OK) {
        struct tm timeinfo;
        localtime_r(&reading.timestamp, &timeinfo);
        char time_str[64];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &timeinfo);

        ESP_LOGI(TAG, "========== LAST BATTERY READING ==========");
        ESP_LOGI(TAG, "Timestamp:       %s", time_str);
        ESP_LOGI(TAG, "ADC Raw:         %lu (0-4095)", (unsigned long)reading.adc_raw);
        ESP_LOGI(TAG, "ADC Calibrated:  %lu mV", (unsigned long)reading.voltage_mv);
        ESP_LOGI(TAG, "Battery Voltage: %.2f V", reading.actual_voltage);
        ESP_LOGI(TAG, "Battery Percent: %.1f%%", reading.percentage);
        ESP_LOGI(TAG, "==========================================");
    } else if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "No battery reading found in NVS (device not yet run on battery)");
    } else {
        ESP_LOGE(TAG, "Failed to retrieve battery reading from NVS: %s", esp_err_to_name(err));
    }
}
