#idf_component_register(SRCS "main.c" "src/disc_nimble.c" "src/ble_nimble.c" "src/parser.c" "src/device_nvs.c" "src/crypto.c" "src/ble_callbacks.c" "src/request.c"
#                    INCLUDE_DIRS "." "./include"
#                    REQUIRES bt nvs_flash mbedtls esp_http_client)

idf_component_register(SRCS "main.c" "src/ble.c" "src/ble_worker.c" "src/device.c" "src/disc.c" "src/parser.c" "src/crypto.c" "src/request.c" "src/request_worker.c" "src/crypto_worker.c" "src/crypto_init" "src/cJSON.c"
                    INCLUDE_DIRS "." "./include/ble" "./include/crypto" "./include/parser" "./include/misc" "./include/request"
                    REQUIRES bt nvs_flash mbedtls esp_http_client driver)