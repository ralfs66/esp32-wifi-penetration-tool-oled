/**
 * @file main.c
 * @author risinek (risinek@gmail.com)
 * @date 2021-04-03
 * @copyright Copyright (c) 2021
 * 
 * @brief Main file used to setup ESP32 into initial state
 * 
 * Starts management AP and webserver  
 */

#include <stdio.h>
#include <string.h>

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#include "esp_event.h"
#include "driver/gpio.h"
#include "nvs_flash.h"
#include "esp_wifi.h"

#include "attack.h"
#include "wifi_controller.h"
#include "webserver.h"
#include "oled_display.h"
#include "ap_scanner.h"

static const char* TAG = "main";

// Define the BOOT button GPIO (usually GPIO0 on ESP32 dev boards)
#define BOOT_BUTTON_GPIO 0

void app_main(void)
{
    ESP_LOGD(TAG, "app_main started");
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Wi-Fi init
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_start());

    wifictl_mgmt_ap_start();
    webserver_run();

    vTaskDelay(pdMS_TO_TICKS(500)); // 500ms delay

    // Initialize OLED display and buttons
    ESP_ERROR_CHECK(oled_display_init());
    oled_display_text("SMARTBOX.LV\nWIFI\nSTOPPER");

    // Initial scan
    ESP_LOGI(TAG, "Starting Wi-Fi scan...");
    scan_and_display_networks();

    while (1) {
        // Handle button presses (up/down/select)
        handle_buttons();
        
        // Small delay to prevent CPU hogging
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
