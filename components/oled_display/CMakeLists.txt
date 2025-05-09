idf_component_register(
    SRCS "oled_display.c"
    INCLUDE_DIRS "include" "${CMAKE_CURRENT_SOURCE_DIR}/../../main" "../wifi_controller"
    REQUIRES driver esp_wifi esp_event main esp_timer
) 