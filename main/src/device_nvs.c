#include "device.h"

#include "nvs_flash.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"

#define WIFI_SSID      "myuser"
#define WIFI_PASS      "mypass"

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
      esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
				xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
		} else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
				xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
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
	
	ESP_ERROR_CHECK(esp_netif_init());

  // Create default event loop
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  // Create WiFi station
  esp_netif_create_default_wifi_sta();

  // Initialize WiFi
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  // Register event handler
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT,
                                             ESP_EVENT_ANY_ID,
                                             &wifi_event_handler,
                                             NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT,
                                             IP_EVENT_STA_GOT_IP,
                                             &wifi_event_handler,
                                             NULL));

  // Configure WiFi
  wifi_config_t wifi_config = {
      .sta = {
          .ssid = WIFI_SSID,
          .password = WIFI_PASS,
					.threshold.authmode = WIFI_AUTH_WPA2_PSK,
//					.sae_pwe_h2e = ESP_WIFI_SAE_MODE,
//					.sae_h2e_identifier = EXAMPLE_H2E_IDENTIFIER,
      },
  };

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());

  ESP_LOGI(TAG, "WiFi init finished.");
	
	EventBits_t bits = xEventGroupWaitBits(
	    s_wifi_event_group,
	    WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
	    pdFALSE,
	    pdFALSE,
	    portMAX_DELAY
	);

	if (bits & WIFI_CONNECTED_BIT) {
	    ESP_LOGI("wifi", "Connected to WiFi");
	    return true;
	} else {
	    ESP_LOGE("wifi", "Failed to connect");
	    return false;
	}
  
  return true;
}
