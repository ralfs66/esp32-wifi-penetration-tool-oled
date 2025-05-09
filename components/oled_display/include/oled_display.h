#pragma once

#include "esp_err.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "esp_wifi.h"

/**
 * @brief Initialize the OLED display
 * 
 * @return esp_err_t ESP_OK on success, otherwise an error code
 */
esp_err_t oled_display_init(void);

/**
 * @brief Display text on the OLED display
 * 
 * @param text Text to display
 * @return esp_err_t ESP_OK on success, otherwise an error code
 */
esp_err_t oled_display_text(const char* text);

/**
 * @brief Handle button presses for network selection and attack control
 */
void handle_buttons(void);

/**
 * @brief Add a network to the display list
 * 
 * @param ssid Network SSID to add
 */
void add_network(const char* ssid);

/**
 * @brief Clear the network list
 */
void clear_networks(void);

/**
 * @brief Scan for networks and display them
 */
void scan_and_display_networks(void); 