#include "request2.h"
#include "freertos/idf_additions.h"
#include "request_worker2.h"
#include "ble2.h"

#include "esp_log.h"
#include "esp_http_client.h"

/* Static + Global Variables */

static const char *TAG = "wifi station";

QueueHandle_t request_worker_queue = NULL;

/* Request Functions */

request_status_t upload_batch(request_work_item_t *batch, size_t batch_len)
{
	payloads_received += batch_len;
//	printf("uploaded %d reports\n", batch_len);

//	esp_http_client_config_t config = {
//		.url = "http://192.168.0.172:3000/send",
//	};
//	esp_http_client_handle_t client = esp_http_client_init(&config);
//
//	esp_http_client_set_method(client, HTTP_METHOD_POST);
//	esp_http_client_set_header(client, "Content-Type", "application/json");
//	
//	char data[] = "{\"title\": \"batch upload!\"}";
//
//	esp_http_client_set_post_field(client, data, sizeof(data)-1);
//	esp_err_t err = esp_http_client_perform(client);
//
//	if (err == ESP_OK) {
//	    ESP_LOGI(TAG, "HTTPS Status = %d, content_length = %"PRId64,
//	            esp_http_client_get_status_code(client),
//	            esp_http_client_get_content_length(client));
//	} else {
//	    ESP_LOGE(TAG, "Error perform http request %s", esp_err_to_name(err));
//	}
//
//	esp_http_client_cleanup(client);
//	esp_http_client_close(client);
	
	return REQUEST_SUCCESS;
}

request_status_t upload_lost_details(request_lost_payload_t *payload)
{
	esp_http_client_config_t config = {
		.url = "http://192.168.0.172:3000/send",
	};
	esp_http_client_handle_t client = esp_http_client_init(&config);

	esp_http_client_set_method(client, HTTP_METHOD_POST);
	esp_http_client_set_header(client, "Content-Type", "application/json");
	
//	uint8_t data[128];
//	char hex[65]; 
//
//	for (int i = 0; i < 32; i++) {
//	    sprintf(&hex[i * 2], "%02X",
//	            payload->finder_key->raw.data[i]);
//	}
//	hex[64] = '\0';
//
//	int len = snprintf((char *)data, sizeof(data),
//	                   "{\"key\":\"%s\"}",
//	                   hex);
//
//	if (len < 0 || len >= sizeof(data)) {
//	    return -1;
//	}

	char data[] = "{\"title\": \"received lost msg!\"}";

	esp_http_client_set_post_field(client, data, sizeof(data)-1);
	esp_err_t err = esp_http_client_perform(client);
	
	if (err == ESP_OK) {
	    ESP_LOGI(TAG, "HTTPS Status = %d, content_length = %"PRId64,
	            esp_http_client_get_status_code(client),
	            esp_http_client_get_content_length(client));
	} else {
	    ESP_LOGE(TAG, "Error perform http request %s", esp_err_to_name(err));
	}
	esp_http_client_cleanup(client);

  return REQUEST_SUCCESS;
}

request_status_t request_init()
{
	request_worker_queue =
	    xQueueCreate(REQUEST_QUEUE_LEN, sizeof(request_work_item_t));

	if (!request_worker_queue) {
	  ESP_LOGE(TAG, "Failed to create Request worker queue");
	  return REQUEST_ERR_NO_MEMORY;
	}

	xTaskCreatePinnedToCore(
		request_worker_task,
		"request_worker",
		8192,
		NULL,
		14,
		NULL,
		1
	);
			
	return REQUEST_SUCCESS;
}