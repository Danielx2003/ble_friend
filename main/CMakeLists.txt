#idf_component_register(SRCS "main.c" "src/disc_nimble.c" "src/ble_nimble.c" "src/parser.c" "src/device_nvs.c" "src/crypto.c" "src/ble_callbacks.c" "src/request.c"
#                    INCLUDE_DIRS "." "./include"
#                    REQUIRES bt nvs_flash mbedtls esp_http_client)

idf_component_register(SRCS "main.c" "src2/ble2.c" "src2/ble_worker2.c" "src2/device2.c"
                    INCLUDE_DIRS "." "./include2"
                    REQUIRES bt nvs_flash mbedtls esp_http_client)