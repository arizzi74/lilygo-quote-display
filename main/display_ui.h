#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize the e-paper display
 */
void display_init(void);

/**
 * Display provisioning mode message
 * Shows instructions to connect to the AP for WiFi configuration
 *
 * @param ap_name Name of the access point to display
 */
void display_provisioning_mode(const char* ap_name);

/**
 * Display connected mode with quote, author and datetime
 * Shows quote centered with text wrapping, author in parentheses in smaller font,
 * and datetime in small font at bottom-left
 *
 * @param quote Quote text to display in center
 * @param author Author name to display in parentheses below quote
 * @param datetime_text Datetime string to display at bottom-left
 */
void display_connected_mode(const char* quote, const char* author, const char* datetime_text);

/**
 * Display connecting status
 * Shows message that device is attempting to connect to WiFi
 *
 * @param ssid SSID being connected to
 */
void display_connecting(const char* ssid);

/**
 * Display loading screen with random gerund
 * Shows 256x256 logo on left and "<gerund>..." centered on right
 * Used when device wakes up to fetch a new quote
 *
 * @param gerund Gerund word to display (e.g., "Thinking")
 */
void display_loading(const char* gerund);

/**
 * Display network reset confirmation message
 * Shows 256x256 logo on left and reset instructions
 * Prompts user to press button 3 times in 10 seconds to confirm reset
 */
void display_reset_confirmation(void);

#ifdef __cplusplus
}
#endif
