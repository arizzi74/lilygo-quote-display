#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"
#include <stdbool.h>

/**
 * Initialize the sleep manager
 * Configures wake sources (timer and button)
 */
void sleep_manager_init(void);

/**
 * Enter deep sleep for specified duration
 * Configures timer wake and button wake (GPIO 39)
 *
 * @param sleep_time_sec Sleep duration in seconds
 */
void sleep_manager_enter_deep_sleep(uint32_t sleep_time_sec);

/**
 * Check if device woke from deep sleep
 *
 * @return true if woke from deep sleep, false if cold boot
 */
bool sleep_manager_is_wakeup_from_sleep(void);

/**
 * Check if device woke from button press (GPIO 39)
 *
 * @return true if woke from button, false otherwise
 */
bool sleep_manager_is_wakeup_from_button(void);

/**
 * Check if device woke from reset button press (GPIO 35)
 *
 * @return true if woke from reset button, false otherwise
 */
bool sleep_manager_is_wakeup_from_reset_button(void);

#ifdef __cplusplus
}
#endif
