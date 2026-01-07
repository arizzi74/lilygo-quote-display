#include "battery.h"
#include "epdiy.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "BATTERY";

// Hardware configuration
#define BATT_PIN_GPIO 36
#define BATT_ADC_CHANNEL ADC1_CHANNEL_0    // GPIO 36 = ADC1_CH0
#define BATT_ADC_ATTEN ADC_ATTEN_DB_11     // 0-3.3V range
#define BATT_ADC_WIDTH ADC_WIDTH_BIT_12    // 12-bit resolution (0-4095)
#define BATT_SAMPLES 64                     // Number of samples to average

// Battery characteristics
#define BATT_VOLTAGE_DIVIDER 2.0           // Hardware voltage divider ratio
#define BATT_MIN_VOLTAGE 3.7               // LiPo minimum voltage
#define BATT_MAX_VOLTAGE 4.2               // LiPo maximum voltage

// ADC calibration
static esp_adc_cal_characteristics_t adc_chars;
static bool initialized = false;

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
    vTaskDelay(pdMS_TO_TICKS(50));  // Wait for voltage to stabilize

    // Read multiple samples and average
    uint32_t adc_sum = 0;
    for (int i = 0; i < BATT_SAMPLES; i++) {
        int raw = adc1_get_raw(BATT_ADC_CHANNEL);
        if (raw < 0) {
            ESP_LOGE(TAG, "ADC read error: %d", raw);
            epd_poweroff();
            return -1.0;
        }
        adc_sum += (uint32_t)raw;
    }
    uint32_t adc_average = adc_sum / BATT_SAMPLES;

    // Power off EPD
    epd_poweroff();

    // Convert ADC reading to voltage (in millivolts) using calibration
    uint32_t voltage_mv = esp_adc_cal_raw_to_voltage(adc_average, &adc_chars);

    // Account for voltage divider (2:1 ratio)
    float actual_voltage = ((float)voltage_mv / 1000.0) * BATT_VOLTAGE_DIVIDER;

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

    // Calculate percentage based on LiPo discharge curve
    // 3.7V = 0%, 4.2V = 100%
    float percentage = ((voltage - BATT_MIN_VOLTAGE) / (BATT_MAX_VOLTAGE - BATT_MIN_VOLTAGE)) * 100.0;

    // Clamp to 0-100% range (handle charging spikes and deep discharge)
    if (percentage > 100.0) {
        percentage = 100.0;
    } else if (percentage < 0.0) {
        percentage = 0.0;
    }

    ESP_LOGI(TAG, "Battery percentage: %.1f%%", percentage);

    return percentage;
}
