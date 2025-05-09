#include "oled_display.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "esp_wifi.h"
#include "attack.h"
#include "attack_dos.h"
#include "ap_scanner.h"
#include <string.h>
#include "esp_timer.h"

static const char *TAG = "oled_display";

#define I2C_MASTER_SCL_IO           22      // GPIO number for I2C master clock
#define I2C_MASTER_SDA_IO           21      // GPIO number for I2C master data
#define I2C_MASTER_NUM              0       // I2C master number
#define I2C_MASTER_FREQ_HZ          400000  // I2C master clock frequency
#define OLED_DISPLAY_ADDR           0x3C    // OLED display I2C address

// Button pins
#define BUZZER_PIN   26    // Buzzer connected to pin 26
#define RELAY_PIN    33    // Relay connected to pin 33
#define BUTTON1_PIN  12    // Red button
#define BUTTON2_PIN  13    // Green button
#define BOOT_BUTTON  0     // Select button

// Network selection state
#define MAX_NETWORKS 10
static wifi_ap_record_t network_list[MAX_NETWORKS];
static int network_count = 0;
static int selected_network = 0;

// SSD1306 commands
#define SSD1306_COMMAND             0x00
#define SSD1306_DATA                0x40

// Display dimensions
#define SSD1306_WIDTH              128
#define SSD1306_HEIGHT             64
#define SSD1306_PAGES              (SSD1306_HEIGHT / 8)

// Display buffer
static uint8_t display_buffer[SSD1306_WIDTH * SSD1306_PAGES];

// Function prototypes
static esp_err_t ssd1306_write_command(uint8_t command);
static esp_err_t ssd1306_write_data(uint8_t* data, size_t size);
static void ssd1306_init_commands(void);
static void init_buttons(void);
static void update_network_display(void);

// Forward declaration of scan_and_display_networks
void scan_and_display_networks(void);

// Initialize buttons
static void init_buttons(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BUTTON1_PIN) | (1ULL << BUTTON2_PIN) | (1ULL << BOOT_BUTTON),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
}

// Scan for networks and display them
void scan_and_display_networks(void) {
    // Use the wifi_controller scan logic
    wifictl_scan_nearby_aps();

    // Get the results
    const wifictl_ap_records_t *ap_records = wifictl_get_ap_records();

    // Clear local list
    clear_networks();

    // Copy up to MAX_NETWORKS SSIDs to the OLED list
    for (int i = 0; i < ap_records->count && i < MAX_NETWORKS; i++) {
        network_list[i] = ap_records->records[i];
    }
    network_count = (ap_records->count < MAX_NETWORKS) ? ap_records->count : MAX_NETWORKS;
    selected_network = 0;
    update_network_display();
}

// Update display to show only the currently selected network
static void update_network_display(void) {
    char display_text[128] = "";
    int start = 0;
    int end = network_count < 8 ? network_count : 8;  // Show up to 8 networks at once

    // If more than 8, scroll window to keep selected in view
    if (selected_network >= 8) {
        start = selected_network - 7;
        end = selected_network + 1;
        if (end > network_count) {
            end = network_count;
            start = end - 8;
        }
    }

    for (int i = start; i < end; i++) {
        char line[36];
        snprintf(line, sizeof(line), "%c%s\n", (i == selected_network) ? '>' : ' ', network_list[i].ssid);
        strncat(display_text, line, sizeof(display_text) - strlen(display_text) - 1);
    }
    oled_display_text(display_text);
}

// Handle button presses: short press toggles, long press attacks
void handle_buttons(void) {
    static bool button1_pressed = false;
    static bool button2_pressed = false;
    static bool boot_pressed = false;
    static int64_t press_start_time = 0;
    static bool attack_started = false;
    const int64_t LONG_PRESS_MS = 800; // 800ms for long press

    bool button1 = !gpio_get_level(BUTTON1_PIN); // UP
    bool button2 = !gpio_get_level(BUTTON2_PIN); // DOWN
    bool boot = !gpio_get_level(BOOT_BUTTON);    // SELECT

    // UP
    if (button1 && !button1_pressed) {
        if (network_count > 0) {
            selected_network = (selected_network - 1 + network_count) % network_count;
            update_network_display();
        }
        button1_pressed = true;
    } else if (!button1) {
        button1_pressed = false;
    }

    // DOWN
    if (button2 && !button2_pressed) {
        if (network_count > 0) {
            selected_network = (selected_network + 1) % network_count;
            update_network_display();
        }
        button2_pressed = true;
    } else if (!button2) {
        button2_pressed = false;
    }

    // BOOT (long press for attack)
    if (boot && !boot_pressed) {
        boot_pressed = true;
        press_start_time = esp_timer_get_time() / 1000; // ms
        attack_started = false;
    } else if (boot && boot_pressed && !attack_started) {
        int64_t press_duration = (esp_timer_get_time() / 1000) - press_start_time;
        if (press_duration >= LONG_PRESS_MS && network_count > 0) {
            // Long press: start attack
            char attack_msg[64];
            snprintf(attack_msg, sizeof(attack_msg), "ATTACKING:\n%s", network_list[selected_network].ssid);
            oled_display_text(attack_msg);
            attack_config_t attack_config = {
                .type = ATTACK_TYPE_DOS,
                .method = ATTACK_DOS_METHOD_COMBINE_ALL,
                .timeout = 0
            };
            attack_config.ap_record = &network_list[selected_network];
            attack_dos_start(&attack_config);
            attack_started = true;
        }
    } else if (!boot && boot_pressed) {
        boot_pressed = false;
    }
}

// Add network to the list
void add_network(const char* ssid) {
    if (network_count < MAX_NETWORKS) {
        strncpy((char *)network_list[network_count].ssid, ssid, sizeof(network_list[0].ssid) - 1);
        network_list[network_count].ssid[sizeof(network_list[0].ssid) - 1] = '\0';
        network_count++;
        update_network_display();
    }
}

// Clear network list
void clear_networks(void) {
    network_count = 0;
    selected_network = 0;
    oled_display_text("SCANNING...");
}

// Initialize display and buttons
esp_err_t oled_display_init(void) {
    ESP_LOGI(TAG, "Initializing OLED display...");

    // Configure I2C
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    ESP_ERROR_CHECK(i2c_param_config(I2C_MASTER_NUM, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0));

    // Initialize SSD1306
    ssd1306_init_commands();
    
    // Clear display
    memset(display_buffer, 0, sizeof(display_buffer));
    ssd1306_write_data(display_buffer, sizeof(display_buffer));

    // Initialize buttons
    init_buttons();

    // Display initial message
    oled_display_text("ESP32-DEAUTH\nSCANNING...");

    ESP_LOGI(TAG, "OLED display initialized successfully");
    return ESP_OK;
}

static void ssd1306_init_commands(void) {
    // Display initialization commands
    ssd1306_write_command(0xAE); // Display off
    ssd1306_write_command(0xD5); // Set display clock
    ssd1306_write_command(0x80);
    ssd1306_write_command(0xA8); // Set multiplex ratio
    ssd1306_write_command(0x3F);
    ssd1306_write_command(0xD3); // Set display offset
    ssd1306_write_command(0x00);
    ssd1306_write_command(0x40 | 0x00); // Set display start line to 0
    ssd1306_write_command(0x8D); // Set charge pump
    ssd1306_write_command(0x14);
    ssd1306_write_command(0x20); // Set memory addressing mode
    ssd1306_write_command(0x00);
    ssd1306_write_command(0xA1); // Segment remap (mirror horizontally)
    ssd1306_write_command(0xC8); // COM scan direction (mirror vertically)
    ssd1306_write_command(0x81); // Set contrast
    ssd1306_write_command(0x7F); // Reduced contrast
    ssd1306_write_command(0xD9); // Set pre-charge period
    ssd1306_write_command(0xF1);
    ssd1306_write_command(0xDB); // Set VCOMH deselect level
    ssd1306_write_command(0x20); // Reduced VCOMH level
    ssd1306_write_command(0xA4); // Display from RAM
    ssd1306_write_command(0xA6); // Normal display
    ssd1306_write_command(0xAF); // Display on
}

static esp_err_t ssd1306_write_command(uint8_t command) {
    uint8_t write_buf[2] = {SSD1306_COMMAND, command};
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (OLED_DISPLAY_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write(cmd, write_buf, sizeof(write_buf), true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    return ret;
}

static esp_err_t ssd1306_write_data(uint8_t* data, size_t size) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (OLED_DISPLAY_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, SSD1306_DATA, true);
    i2c_master_write(cmd, data, size, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    return ret;
}

// Simple 5x7 font data (only uppercase letters and numbers)
static const uint8_t font_5x7[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, // Space
    0x00, 0x00, 0x5F, 0x00, 0x00, // !
    0x00, 0x07, 0x00, 0x07, 0x00, // "
    0x14, 0x7F, 0x14, 0x7F, 0x14, // #
    0x24, 0x2A, 0x7F, 0x2A, 0x12, // $
    0x23, 0x13, 0x08, 0x64, 0x62, // %
    0x36, 0x49, 0x55, 0x22, 0x50, // &
    0x00, 0x05, 0x03, 0x00, 0x00, // '
    0x00, 0x1C, 0x22, 0x41, 0x00, // (
    0x00, 0x41, 0x22, 0x1C, 0x00, // )
    0x08, 0x2A, 0x1C, 0x2A, 0x08, // *
    0x08, 0x08, 0x3E, 0x08, 0x08, // +
    0x00, 0x50, 0x30, 0x00, 0x00, // ,
    0x08, 0x08, 0x08, 0x08, 0x08, // -
    0x00, 0x60, 0x60, 0x00, 0x00, // .
    0x20, 0x10, 0x08, 0x04, 0x02, // /
    0x3E, 0x51, 0x49, 0x45, 0x3E, // 0
    0x00, 0x42, 0x7F, 0x40, 0x00, // 1
    0x42, 0x61, 0x51, 0x49, 0x46, // 2
    0x21, 0x41, 0x45, 0x4B, 0x31, // 3
    0x18, 0x14, 0x12, 0x7F, 0x10, // 4
    0x27, 0x45, 0x45, 0x45, 0x39, // 5
    0x3C, 0x4A, 0x49, 0x49, 0x30, // 6
    0x01, 0x71, 0x09, 0x05, 0x03, // 7
    0x36, 0x49, 0x49, 0x49, 0x36, // 8
    0x06, 0x49, 0x49, 0x29, 0x1E, // 9
    0x00, 0x36, 0x36, 0x00, 0x00, // :
    0x00, 0x56, 0x36, 0x00, 0x00, // ;
    0x00, 0x08, 0x14, 0x22, 0x41, // <
    0x14, 0x14, 0x14, 0x14, 0x14, // =
    0x41, 0x22, 0x14, 0x08, 0x00, // >
    0x02, 0x01, 0x51, 0x09, 0x06, // ?
    0x32, 0x49, 0x79, 0x41, 0x3E, // @
    0x7E, 0x11, 0x11, 0x11, 0x7E, // A
    0x7F, 0x49, 0x49, 0x49, 0x36, // B
    0x3E, 0x41, 0x41, 0x41, 0x22, // C
    0x7F, 0x41, 0x41, 0x22, 0x1C, // D
    0x7F, 0x49, 0x49, 0x49, 0x41, // E
    0x7F, 0x09, 0x09, 0x01, 0x01, // F
    0x3E, 0x41, 0x41, 0x49, 0x7A, // G
    0x7F, 0x08, 0x08, 0x08, 0x7F, // H
    0x00, 0x41, 0x7F, 0x41, 0x00, // I
    0x20, 0x40, 0x41, 0x3F, 0x01, // J
    0x7F, 0x08, 0x14, 0x22, 0x41, // K
    0x7F, 0x40, 0x40, 0x40, 0x40, // L
    0x7F, 0x02, 0x04, 0x02, 0x7F, // M
    0x7F, 0x04, 0x08, 0x10, 0x7F, // N
    0x3E, 0x41, 0x41, 0x41, 0x3E, // O
    0x7F, 0x09, 0x09, 0x09, 0x06, // P
    0x3E, 0x41, 0x51, 0x21, 0x5E, // Q
    0x7F, 0x09, 0x19, 0x29, 0x46, // R
    0x46, 0x49, 0x49, 0x49, 0x31, // S
    0x01, 0x01, 0x7F, 0x01, 0x01, // T
    0x3F, 0x40, 0x40, 0x40, 0x3F, // U
    0x1F, 0x20, 0x40, 0x20, 0x1F, // V
    0x7F, 0x20, 0x18, 0x20, 0x7F, // W
    0x63, 0x14, 0x08, 0x14, 0x63, // X
    0x03, 0x04, 0x78, 0x04, 0x03, // Y
    0x61, 0x51, 0x49, 0x45, 0x43, // Z
};

esp_err_t oled_display_text(const char* text) {
    memset(display_buffer, 0, sizeof(display_buffer));

    int line = 0;  // Start at the top line
    int x = 0;

    for (int i = 0; text[i] != '\0'; i++) {
        if (text[i] == '\n' || x > SSD1306_WIDTH - 6) {
            line++;
            x = 0;
            if (line >= SSD1306_PAGES) break; // Only 8 lines on 64px display
            if (text[i] == '\n') continue;
        }
        char c = text[i];
        // Convert to uppercase if your font only supports uppercase
        if (c >= 'a' && c <= 'z') {
            c = c - 'a' + 'A';
        }
        if (c >= ' ' && c <= 'Z') {
            const uint8_t* char_data = &font_5x7[(c - ' ') * 5];
            for (int j = 0; j < 5; j++) {
                display_buffer[line * SSD1306_WIDTH + x + j] = char_data[j];
            }
            x += 6;
        }
    }

    // Set column address to start at 0
    ssd1306_write_command(0x21);
    ssd1306_write_command(0x00);
    ssd1306_write_command(0x7F);

    return ssd1306_write_data(display_buffer, sizeof(display_buffer));
} 