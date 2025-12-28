idf_component_register(
    SRCS "main.c"
         "display_ui.c"
         "wifi_manager.c"
         "webserver.c"
         "wikiquote.c"
         "sleep_manager.c"
         "gerunds.c"
    INCLUDE_DIRS "."
    REQUIRES epdiy
             nvs_flash
             esp_wifi
             esp_netif
             esp_http_server
             esp_http_client
             esp-tls
             esp_event
             json
             driver
)
