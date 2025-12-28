#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"

/**
 * Initialize the Wikiquote client
 */
esp_err_t wikiquote_init(void);

/**
 * Get a random quote from Quotable.io API
 *
 * @param author_name Not used (pass NULL) - gets random quote from any author
 * @param quote_buffer Buffer to store the quote text
 * @param buffer_size Size of the quote buffer
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t wikiquote_get_random_quote(const char* author_name, char* quote_buffer, size_t buffer_size);

/**
 * Get a random quote with author from Quotable.io API
 *
 * @param quote_buffer Buffer to store the quote text
 * @param quote_size Size of the quote buffer
 * @param author_buffer Buffer to store the author name
 * @param author_size Size of the author buffer
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t wikiquote_get_random_quote_with_author(char* quote_buffer, size_t quote_size,
                                                  char* author_buffer, size_t author_size);

#ifdef __cplusplus
}
#endif
