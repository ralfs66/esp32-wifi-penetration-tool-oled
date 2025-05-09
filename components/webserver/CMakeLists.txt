idf_component_register(SRCS "webserver.c"
                    INCLUDE_DIRS "interface"
                    REQUIRES hccapx_serializer frame_analyzer
                    PRIV_REQUIRES pcap_serializer esp_http_server wifi_controller main)