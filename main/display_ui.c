#include "display_ui.h"
#include "epdiy.h"
#include "firasans_20.h"
#include "firasans_12.h"
#include "opensans8.h"
#include "wm_logo_64.h"
#include "wm_logo_256.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "DISPLAY_UI";

// High-level EPD state
static EpdiyHighlevelState hl;

void display_init(void) {
    ESP_LOGI(TAG, "Initializing e-paper display...");

    // Initialize EPD with Lilygo T5-4.7 board and ED047TC1 display
    epd_init(&epd_board_lilygo_t5_47, &ED047TC1, EPD_LUT_64K);

    // VCOM voltage is hardware-set on this board, no software config needed

    // Initialize high-level state with builtin waveform
    hl = epd_hl_init(EPD_BUILTIN_WAVEFORM);

    // Set landscape orientation (960x540)
    epd_set_rotation(EPD_ROT_LANDSCAPE);

    ESP_LOGI(TAG, "Display initialized: %dx%d",
             epd_rotated_display_width(),
             epd_rotated_display_height());
}

void display_provisioning_mode(const char* ap_name) {
    ESP_LOGI(TAG, "Displaying provisioning mode message...");

    uint8_t* fb = epd_hl_get_framebuffer(&hl);
    if (fb == NULL) {
        ESP_LOGE(TAG, "Failed to get framebuffer!");
        return;
    }

    // Clear framebuffer to white
    epd_clear();

    // Power on display
    epd_poweron();
    vTaskDelay(pdMS_TO_TICKS(100));

    EpdFontProperties props = {
        .fg_color = 0,      // Black (4-bit: 0x0)
        .bg_color = 15,     // White (4-bit: 0xF)
        .fallback_glyph = 0,
        .flags = 0
    };

    // Draw 256x256 logo on the left, centered vertically
    EpdRect logo_area = {
        .x = 80,
        .y = (540 - 256) / 2,  // Center vertically (142)
        .width = wm_logo_256_width,
        .height = wm_logo_256_height
    };
    epd_copy_to_framebuffer(logo_area, wm_logo_256_data, fb);

    // Display main message (to the right of the logo)
    // Use better word wrapping
    const char* msg1 = "Connect to";
    int x = 380, y = 200;
    epd_write_string(&FiraSans_20, msg1, &x, &y, fb, &props);

    // Display SSID on second line
    char msg2[64];
    snprintf(msg2, sizeof(msg2), "'%s' network", ap_name);
    x = 380; y = 235;
    epd_write_string(&FiraSans_20, msg2, &x, &y, fb, &props);

    // Display third line
    const char* msg3 = "to configure WiFi";
    x = 380; y = 270;
    epd_write_string(&FiraSans_20, msg3, &x, &y, fb, &props);

    // Display URL instruction
    const char* msg4 = "Open: http://192.168.4.1";
    x = 380; y = 310;
    epd_write_string(&FiraSans_12, msg4, &x, &y, fb, &props);

    // Update screen
    enum EpdDrawError err = epd_hl_update_screen(&hl, MODE_GC16, 25);
    if (err != EPD_DRAW_SUCCESS) {
        ESP_LOGE(TAG, "Display update failed with error: %d", err);
    }

    // Power off display to save energy
    epd_poweroff();
}

void display_connected_mode(const char* quote, const char* author, const char* datetime_text) {
    ESP_LOGI(TAG, "Displaying quote with author...");

    uint8_t* fb = epd_hl_get_framebuffer(&hl);
    if (fb == NULL) {
        ESP_LOGE(TAG, "Failed to get framebuffer!");
        return;
    }

    // Power on display
    epd_poweron();
    vTaskDelay(pdMS_TO_TICKS(100));

    // IMPORTANT: Complete white screen refresh to remove all ghosting
    ESP_LOGI(TAG, "Clearing screen with white refresh...");

    // Fill framebuffer with white
    memset(fb, 0xFF, epd_width() / 2 * epd_height());

    // Update display with white screen (full refresh)
    enum EpdDrawError err = epd_hl_update_screen(&hl, MODE_GC16, 25);
    if (err != EPD_DRAW_SUCCESS) {
        ESP_LOGE(TAG, "White screen update failed: %d", err);
    }
    vTaskDelay(pdMS_TO_TICKS(500));

    // Now clear framebuffer for new content
    epd_clear();

    EpdFontProperties props = {
        .fg_color = 0,      // Black (4-bit: 0x0)
        .bg_color = 15,     // White (4-bit: 0xF)
        .fallback_glyph = 0,
        .flags = 0
    };

    // Draw quote centered with basic wrapping
    // For simplicity, we'll use a fixed width and wrap words
    int max_width = 860;  // Leave 50px margins on each side
    int line_height = 35;
    int start_y = 150;    // Start position for first line

    // Simple word wrapping
    char quote_copy[512];
    strncpy(quote_copy, quote, sizeof(quote_copy) - 1);
    quote_copy[sizeof(quote_copy) - 1] = '\0';

    char *word = strtok(quote_copy, " ");
    char line[400] = "";
    int y = start_y;

    while (word != NULL) {
        // Check if adding this word would fit in the line
        size_t current_len = strlen(line);
        size_t word_len = strlen(word);
        size_t space_needed = current_len + (current_len > 0 ? 1 : 0) + word_len;

        // Build test line if it would fit in buffer
        char test_line[400] = "";
        if (space_needed < sizeof(test_line)) {
            if (current_len > 0) {
                strcpy(test_line, line);
                strcat(test_line, " ");
                strcat(test_line, word);
            } else {
                strcpy(test_line, word);
            }

            int x = 0, text_y = 0;
            int text_x1, text_y1, text_width, text_height;
            epd_get_text_bounds(&FiraSans_20, test_line, &x, &text_y,
                                &text_x1, &text_y1, &text_width, &text_height, &props);

            if (text_width > max_width && current_len > 0) {
                // Line is too long, draw current line and start new one
                epd_get_text_bounds(&FiraSans_20, line, &x, &text_y,
                                    &text_x1, &text_y1, &text_width, &text_height, &props);
                x = (960 - text_width) / 2;  // Center the line
                epd_write_string(&FiraSans_20, line, &x, &y, fb, &props);
                y += line_height;

                // Start new line with current word
                strcpy(line, word);
            } else {
                // Add word to current line
                strcpy(line, test_line);
            }
        }

        word = strtok(NULL, " ");
    }

    // Draw final line of quote
    if (strlen(line) > 0) {
        int x = 0, text_y = 0;
        int text_x1, text_y1, text_width, text_height;
        epd_get_text_bounds(&FiraSans_20, line, &x, &text_y,
                            &text_x1, &text_y1, &text_width, &text_height, &props);
        x = (960 - text_width) / 2;
        epd_write_string(&FiraSans_20, line, &x, &y, fb, &props);
        y += line_height;  // Move down for author
    }

    // Draw author in parentheses with same font as quote (FiraSans_20)
    y += 15;  // Extra spacing between quote and author
    char author_text[256];
    snprintf(author_text, sizeof(author_text), "(%s)", author);

    int x = 0, text_y = 0;
    int text_x1, text_y1, text_width, text_height;
    epd_get_text_bounds(&FiraSans_20, author_text, &x, &text_y,
                        &text_x1, &text_y1, &text_width, &text_height, &props);
    x = (960 - text_width) / 2;  // Center the author
    epd_write_string(&FiraSans_20, author_text, &x, &y, fb, &props);

    // Datetime text at bottom-left corner (OpenSans8)
    x = 10;         // Left edge
    y = 540 - 15;   // Bottom edge
    epd_write_string(&OpenSans8, datetime_text, &x, &y, fb, &props);

    // Draw logo at bottom-right corner (64x64 icon)
    EpdRect logo_area = {
        .x = 960 - 64 - 10,  // 10px margin from right edge
        .y = 540 - 64 - 10,  // 10px margin from bottom edge
        .width = wm_logo_64_width,
        .height = wm_logo_64_height
    };
    epd_copy_to_framebuffer(logo_area, wm_logo_64_data, fb);

    // Update screen with new content
    err = epd_hl_update_screen(&hl, MODE_GC16, 25);
    if (err != EPD_DRAW_SUCCESS) {
        ESP_LOGE(TAG, "Display update failed with error: %d", err);
    } else {
        ESP_LOGI(TAG, "Display updated successfully!");
    }

    // Power off display to save energy
    epd_poweroff();
}

void display_connecting(const char* ssid) {
    ESP_LOGI(TAG, "Displaying connecting message...");

    uint8_t* fb = epd_hl_get_framebuffer(&hl);
    if (fb == NULL) {
        ESP_LOGE(TAG, "Failed to get framebuffer!");
        return;
    }

    // Clear framebuffer to white
    epd_clear();

    // Power on display
    epd_poweron();
    vTaskDelay(pdMS_TO_TICKS(100));

    EpdFontProperties props = {
        .fg_color = 0,      // Black (4-bit: 0x0)
        .bg_color = 15,     // White (4-bit: 0xF)
        .fallback_glyph = 0,
        .flags = 0
    };

    // Draw 256x256 logo on the left, centered vertically
    EpdRect logo_area = {
        .x = 80,
        .y = (540 - 256) / 2,  // Center vertically (142)
        .width = wm_logo_256_width,
        .height = wm_logo_256_height
    };
    epd_copy_to_framebuffer(logo_area, wm_logo_256_data, fb);

    // Display connecting message (to the right of the logo)
    char msg[128];
    snprintf(msg, sizeof(msg), "Connecting to: %s", ssid);
    int x = 380, y = 250;
    epd_write_string(&FiraSans_20, msg, &x, &y, fb, &props);

    // Update screen
    enum EpdDrawError err = epd_hl_update_screen(&hl, MODE_GC16, 25);
    if (err != EPD_DRAW_SUCCESS) {
        ESP_LOGE(TAG, "Display update failed with error: %d", err);
    }

    // Power off display to save energy
    epd_poweroff();
}

void display_loading(const char* gerund) {
    ESP_LOGI(TAG, "Displaying loading screen with gerund: %s", gerund);

    uint8_t* fb = epd_hl_get_framebuffer(&hl);
    if (fb == NULL) {
        ESP_LOGE(TAG, "Failed to get framebuffer!");
        return;
    }

    // Clear framebuffer to white
    epd_clear();

    // Power on display
    epd_poweron();
    vTaskDelay(pdMS_TO_TICKS(100));

    EpdFontProperties props = {
        .fg_color = 0,      // Black (4-bit: 0x0)
        .bg_color = 15,     // White (4-bit: 0xF)
        .fallback_glyph = 0,
        .flags = 0
    };

    // Draw 256x256 logo on the left, centered vertically
    EpdRect logo_area = {
        .x = 80,
        .y = (540 - 256) / 2,  // Center vertically (142)
        .width = wm_logo_256_width,
        .height = wm_logo_256_height
    };
    epd_copy_to_framebuffer(logo_area, wm_logo_256_data, fb);

    // Create the message with three dots
    char message[64];
    snprintf(message, sizeof(message), "%s...", gerund);

    // Position text to the right of the logo with spacing
    // Logo ends at x=336 (80+256), add ~44px spacing
    int x = 380;
    int y = 270;  // Center vertically

    epd_write_string(&FiraSans_20, message, &x, &y, fb, &props);

    // Update screen
    enum EpdDrawError err = epd_hl_update_screen(&hl, MODE_GC16, 25);
    if (err != EPD_DRAW_SUCCESS) {
        ESP_LOGE(TAG, "Display update failed with error: %d", err);
    }

    // Power off display to save energy
    epd_poweroff();
}

void display_reset_confirmation(void) {
    ESP_LOGI(TAG, "Displaying network reset confirmation message...");

    uint8_t* fb = epd_hl_get_framebuffer(&hl);
    if (fb == NULL) {
        ESP_LOGE(TAG, "Failed to get framebuffer!");
        return;
    }

    // Clear framebuffer to white
    epd_clear();

    // Power on display
    epd_poweron();
    vTaskDelay(pdMS_TO_TICKS(100));

    EpdFontProperties props = {
        .fg_color = 0,      // Black (4-bit: 0x0)
        .bg_color = 15,     // White (4-bit: 0xF)
        .fallback_glyph = 0,
        .flags = 0
    };

    // Draw 256x256 logo on the left, centered vertically
    EpdRect logo_area = {
        .x = 80,
        .y = (540 - 256) / 2,  // Center vertically (142)
        .width = wm_logo_256_width,
        .height = wm_logo_256_height
    };
    epd_copy_to_framebuffer(logo_area, wm_logo_256_data, fb);

    // Display reset message (to the right of the logo)
    // Use better word wrapping to fit on screen
    const char* msg1 = "To reset network";
    int x = 380, y = 180;
    epd_write_string(&FiraSans_20, msg1, &x, &y, fb, &props);

    const char* msg2 = "configuration press";
    x = 380; y = 215;
    epd_write_string(&FiraSans_20, msg2, &x, &y, fb, &props);

    const char* msg3 = "same button 3 times";
    x = 380; y = 250;
    epd_write_string(&FiraSans_20, msg3, &x, &y, fb, &props);

    const char* msg4 = "in next 10 seconds";
    x = 380; y = 285;
    epd_write_string(&FiraSans_20, msg4, &x, &y, fb, &props);

    const char* msg5 = "or wait to cancel.";
    x = 380; y = 320;
    epd_write_string(&FiraSans_20, msg5, &x, &y, fb, &props);

    // Update screen
    enum EpdDrawError err = epd_hl_update_screen(&hl, MODE_GC16, 25);
    if (err != EPD_DRAW_SUCCESS) {
        ESP_LOGE(TAG, "Display update failed with error: %d", err);
    }

    // Power off display to save energy
    epd_poweroff();
}
