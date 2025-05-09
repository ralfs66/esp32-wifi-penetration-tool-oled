#pragma once

#include "esp_err.h"

esp_err_t oled_display_init(void);
esp_err_t oled_display_text(const char* text);
void handle_buttons(void);
void add_network(const char* ssid);
void clear_networks(void); 