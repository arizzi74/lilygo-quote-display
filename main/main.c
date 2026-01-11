#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "display_ui.h"
#include "wifi_manager.h"
#include "sleep_manager.h"
#include "battery.h"
#include "gerunds.h"
#include "driver/gpio.h"

static const char *TAG = "MAIN";

#define RESET_BUTTON_GPIO GPIO_NUM_35
#define RESET_TIMEOUT_MS 10000  // 10 seconds
#define RESET_PRESS_COUNT 3     // Require 3 button presses

// Monitor reset button for 3 presses within timeout
static bool wait_for_reset_confirmation(void) {
    ESP_LOGI(TAG, "Monitoring reset button for %d presses in %d seconds...",
             RESET_PRESS_COUNT, RESET_TIMEOUT_MS / 1000);

    int press_count = 0;
    TickType_t start_time = xTaskGetTickCount();
    TickType_t timeout_ticks = pdMS_TO_TICKS(RESET_TIMEOUT_MS);
    bool last_state = gpio_get_level(RESET_BUTTON_GPIO);

    while ((xTaskGetTickCount() - start_time) < timeout_ticks) {
        bool current_state = gpio_get_level(RESET_BUTTON_GPIO);

        // Detect button press (transition from high to low, active low button)
        if (last_state == 1 && current_state == 0) {
            press_count++;
            ESP_LOGI(TAG, "Button press detected (%d/%d)", press_count, RESET_PRESS_COUNT);

            if (press_count >= RESET_PRESS_COUNT) {
                ESP_LOGI(TAG, "Reset confirmed! Received %d presses.", press_count);
                return true;
            }

            // Wait for button release to avoid counting same press multiple times
            while (gpio_get_level(RESET_BUTTON_GPIO) == 0) {
                vTaskDelay(pdMS_TO_TICKS(10));
            }
        }

        last_state = current_state;
        vTaskDelay(pdMS_TO_TICKS(50));  // Poll every 50ms
    }

    ESP_LOGI(TAG, "Reset timeout - only %d/%d presses received", press_count, RESET_PRESS_COUNT);
    return false;
}

void app_main(void) {
    ESP_LOGI(TAG, "Starting Lilygo T5-4.7 Quote Display");

    // Initialize NVS (required for WiFi credentials storage)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS partition was truncated, erasing...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "NVS initialized");

    // Print last battery reading from NVS (useful for debugging on battery)
    battery_print_last_reading();

    // Initialize sleep manager
    sleep_manager_init();

    // Check if we woke from deep sleep
    bool is_wakeup = sleep_manager_is_wakeup_from_sleep();
    bool is_button_wake = sleep_manager_is_wakeup_from_button();
    bool is_reset_button_wake = sleep_manager_is_wakeup_from_reset_button();

    if (is_wakeup) {
        if (is_button_wake) {
            ESP_LOGI(TAG, "Woke from button press - fetching new quote immediately");
        } else if (is_reset_button_wake) {
            ESP_LOGI(TAG, "Woke from reset button press - network reset requested");
        } else {
            ESP_LOGI(TAG, "Woke from timer - time for periodic quote update");
        }
    } else {
        ESP_LOGI(TAG, "Cold boot - first run");
    }

    // Initialize e-paper display
    display_init();

    // Handle reset button wake - network configuration reset
    if (is_reset_button_wake) {
        ESP_LOGI(TAG, "Displaying reset confirmation message");
        display_reset_confirmation();

        // Wait for 3 button presses within 10 seconds
        bool reset_confirmed = wait_for_reset_confirmation();

        if (reset_confirmed) {
            ESP_LOGI(TAG, "Network reset confirmed - deleting WiFi credentials");
            wifi_manager_delete_credentials();
            ESP_LOGI(TAG, "Credentials deleted, rebooting...");
            vTaskDelay(pdMS_TO_TICKS(1000));
            esp_restart();
        } else {
            ESP_LOGI(TAG, "Network reset cancelled - rebooting without changes");
            vTaskDelay(pdMS_TO_TICKS(1000));
            esp_restart();
        }
        // Never reached - device restarts above
        return;
    }

    // Show loading screen if waking from sleep (button or timer, not reset)
    if (is_wakeup && !is_reset_button_wake) {
        const char* random_gerund = get_random_gerund();
        ESP_LOGI(TAG, "Displaying loading screen with: %s", random_gerund);
        display_loading(random_gerund);
    }

    // Initialize WiFi manager
    ESP_ERROR_CHECK(wifi_manager_init());

    // Start WiFi manager (silent if waking from sleep)
    ESP_ERROR_CHECK(wifi_manager_start(is_wakeup));

    ESP_LOGI(TAG, "Initialization complete, WiFi manager will handle connection and quote display");

    // Main loop - WiFi manager handles everything via event callbacks
    // After quote is displayed, the connection_setup_task will enter deep sleep
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
