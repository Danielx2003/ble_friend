#include "device2.h"
#include "crypto2.h"
#include "esp_err.h"
#include "nvs.h"
#include "wifi_password.h"

#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_log.h"

static EventGroupHandle_t s_wifi_event_group;
static const char *TAG = "wifi";

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static void wifi_event_handler(void *arg,
                               esp_event_base_t event_base,
                               int32_t event_id,
                               void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
				xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
		} else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
				xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

bool is_ecdsa_keypair_in_storage()
{
	nvs_handle_t handle;
	nvs_type_t type;
	esp_err_t err;
	
	err = nvs_open("crypto", NVS_READONLY, &handle);
	esp_err_t found_ecdsa_pub = nvs_find_key(handle, NVS_ECDSA_PUB_KEY, &type);

	bool found = 
		err == ESP_ERR_NVS_NOT_FOUND 
		|| found_ecdsa_pub == ESP_ERR_NVS_NOT_FOUND ? false 
		: true;

	nvs_close(handle);

	return found;
}

void write_ecdsa_public_key_to_storage(
	crypto_key_t *ecdsa_public_key
)
{
	nvs_handle_t handle;

	esp_err_t err = nvs_open("crypto", NVS_READWRITE, &handle);
	if (err != ESP_OK) { return; }
	nvs_set_blob(handle, NVS_ECDSA_PUB_KEY, ecdsa_public_key->raw.data, ecdsa_public_key->raw.len);
	nvs_commit(handle);
	nvs_close(handle);
}

void load_ecdsa_public_key_from_storage(
	crypto_key_t *ecdsa_public_key
)
{
	nvs_handle_t handle;

	esp_err_t err = nvs_open("crypto", NVS_READONLY, &handle);
	if (err != ESP_OK) { return; }
	nvs_get_blob(handle, NVS_ECDSA_PUB_KEY, ecdsa_public_key->raw.data, &ecdsa_public_key->raw.len);
	nvs_close(handle);
}

bool device_init()
{
	s_wifi_event_group = xEventGroupCreate();

	esp_err_t ret = nvs_flash_init();
	if  (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
	    ESP_ERROR_CHECK(nvs_flash_erase());
	    ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);
	
//	ESP_ERROR_CHECK(esp_netif_init());
//
//  // Create default event loop
//  ESP_ERROR_CHECK(esp_event_loop_create_default());
//
//  // Create WiFi station
//  esp_netif_create_default_wifi_sta();
//
//  // Initialize WiFi
//  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
//  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
//
//  // Register event handler
//  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT,
//                                             ESP_EVENT_ANY_ID,
//                                             &wifi_event_handler,
//                                             NULL));
//  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT,
//                                             IP_EVENT_STA_GOT_IP,
//                                             &wifi_event_handler,
//                                             NULL));
//
//  // Configure WiFi
//  wifi_config_t wifi_config = {
//      .sta = {
//          .ssid = WIFI_SSID,
//          .password = WIFI_PASS,
//					.threshold.authmode = WIFI_AUTH_WPA2_PSK,
//      },
//  };
//
////	wifi_config_t wifi_config = {
////	    .sta = {
////	        .ssid = "eduroam",
////	        .password = "my_password",
////					.threshold.authmode = WIFI_AUTH_WPA2_ENTERPRISE,
////	    },
////	};
//
//  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
//  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
//	ESP_ERROR_CHECK(esp_wifi_start());
//
//  ESP_LOGI(TAG, "WiFi init finished.");
//	
//	EventBits_t bits = xEventGroupWaitBits(
//	    s_wifi_event_group,
//	    WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
//	    pdFALSE,
//	    pdFALSE,
//	    portMAX_DELAY
//	);
//
//	if (bits & WIFI_CONNECTED_BIT) {
//		vEventGroupDelete(s_wifi_event_group);
//		ESP_LOGI("wifi", "Connected to WiFi");
//	  return true;
//	} else {
//		vEventGroupDelete(s_wifi_event_group);
//	  ESP_LOGE("wifi", "Failed to connect");
//	  return true;
//	}
//  
  return true;
}