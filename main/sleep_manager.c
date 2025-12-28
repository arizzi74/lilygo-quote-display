#include "sleep_manager.h"
#include "esp_sleep.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"

static const char *TAG = "SLEEP_MANAGER";

// Button GPIOs for wakeup on Lilygo T5-4.7
#define WAKEUP_BUTTON_GPIO GPIO_NUM_39      // Upper button - quote refresh
#define RESET_BUTTON_GPIO GPIO_NUM_35       // Reset button - network reset

void sleep_manager_init(void) {
    ESP_LOGI(TAG, "Initializing sleep manager...");

    // Configure GPIO 39 (quote refresh button) as input
    // GPIO 39 is input-only, no internal pullup available
    // External pullup resistor should be present on hardware
    gpio_config_t io_conf_39 = {
        .pin_bit_mask = (1ULL << WAKEUP_BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf_39);

    // Configure GPIO 35 (reset button) as input
    // GPIO 35 is input-only, no internal pullup available
    gpio_config_t io_conf_35 = {
        .pin_bit_mask = (1ULL << RESET_BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf_35);

    ESP_LOGI(TAG, "Sleep manager initialized, button wake on GPIO %d and %d",
             WAKEUP_BUTTON_GPIO, RESET_BUTTON_GPIO);
}

void sleep_manager_enter_deep_sleep(uint32_t sleep_time_sec) {
    ESP_LOGI(TAG, "Entering deep sleep for %lu seconds...", sleep_time_sec);

    // Configure timer wakeup
    uint64_t sleep_time_us = (uint64_t)sleep_time_sec * 1000000ULL;
    esp_sleep_enable_timer_wakeup(sleep_time_us);
    ESP_LOGI(TAG, "Timer wakeup configured for %llu microseconds", sleep_time_us);

    // Configure EXT0 wakeup on quote refresh button (low level = button pressed)
    // GPIO 39 is RTC GPIO, can be used for EXT0 wakeup
    esp_sleep_enable_ext0_wakeup(WAKEUP_BUTTON_GPIO, 0);  // 0 = low level (button pressed)
    ESP_LOGI(TAG, "Quote refresh button wakeup configured on GPIO %d (active low)", WAKEUP_BUTTON_GPIO);

    // Configure EXT1 wakeup on reset button (low level = button pressed)
    // GPIO 35 is RTC GPIO, can be used for EXT1 wakeup
    const uint64_t ext1_mask = (1ULL << RESET_BUTTON_GPIO);
    esp_sleep_enable_ext1_wakeup(ext1_mask, ESP_EXT1_WAKEUP_ALL_LOW);
    ESP_LOGI(TAG, "Reset button wakeup configured on GPIO %d (active low)", RESET_BUTTON_GPIO);

    // Isolate GPIO12 pin from external circuits to prevent current leakage
    rtc_gpio_isolate(GPIO_NUM_12);

    ESP_LOGI(TAG, "Entering deep sleep now...");

    // Enter deep sleep
    esp_deep_sleep_start();
}

bool sleep_manager_is_wakeup_from_sleep(void) {
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

    switch (wakeup_reason) {
        case ESP_SLEEP_WAKEUP_TIMER:
            ESP_LOGI(TAG, "Wakeup caused by timer");
            return true;
        case ESP_SLEEP_WAKEUP_EXT0:
            ESP_LOGI(TAG, "Wakeup caused by EXT0 (GPIO %d - quote refresh button)", WAKEUP_BUTTON_GPIO);
            return true;
        case ESP_SLEEP_WAKEUP_EXT1:
            ESP_LOGI(TAG, "Wakeup caused by EXT1 (GPIO %d - reset button)", RESET_BUTTON_GPIO);
            return true;
        case ESP_SLEEP_WAKEUP_UNDEFINED:
        default:
            ESP_LOGI(TAG, "Cold boot (not from deep sleep)");
            return false;
    }
}

bool sleep_manager_is_wakeup_from_button(void) {
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    return (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0);
}

bool sleep_manager_is_wakeup_from_reset_button(void) {
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    return (wakeup_reason == ESP_SLEEP_WAKEUP_EXT1);
}
