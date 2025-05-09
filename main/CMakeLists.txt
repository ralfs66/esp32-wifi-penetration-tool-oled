idf_component_register(
    SRCS "main.c" "attack.c" "attack_dos.c" "attack_handshake.c" "attack_pmkid.c" "attack_method.c"
    INCLUDE_DIRS "."
    REQUIRES wifi_controller webserver wsl_bypasser oled_display pcap_serializer nvs_flash
)
