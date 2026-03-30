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


request_status_t upload_lost_batch(request_lost_payload_t *batch, size_t batch_len)
{
	payloads_received += batch_len;

	esp_http_client_config_t config = {
		.url = "http://192.168.0.172:3000/send",
	};
	esp_http_client_handle_t client = esp_http_client_init(&config);

	esp_http_client_set_method(client, HTTP_METHOD_POST);
	esp_http_client_set_header(client, "Content-Type", "application/octet-stream");
	
	request_lost_batch_wire_t data = {
		.size = batch_len,
	};

	memcpy(data.payloads, batch, sizeof(request_lost_payload_t) * batch_len);
	
	esp_http_client_set_post_field(
		client,
		(char *)&data,
		sizeof(size_t) + sizeof(request_lost_payload_t) * batch_len
	);
	
	printf("Data size: %d - Payload size: %d - Num Payloads: %d\n",
		sizeof(data),
		sizeof(size_t) + sizeof(request_lost_payload_t) * batch_len,
		batch_len
	);

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

request_status_t upload_lost_details(request_lost_payload_t *payload)
{
	esp_http_client_config_t config = {
		.url = "http://192.168.0.172:3000/send",
	};
	esp_http_client_handle_t client = esp_http_client_init(&config);

	esp_http_client_set_method(client, HTTP_METHOD_POST);
	esp_http_client_set_header(client, "Content-Type", "application/octet-stream");
	
	request_lost_wire_t wire_payload;

	memcpy(wire_payload.device_id, payload->device_id, 36);
	memcpy(wire_payload.location, payload->location, 2);
	memcpy(wire_payload.public_key, payload->finder_key->raw.data, 32);
//	wire_payload.public_key_len = payload->finder_key->raw.len;
	memcpy(wire_payload.signature, payload->signature, 64);

	esp_http_client_set_post_field(
		client,
		(char *)&wire_payload,
		sizeof(wire_payload)
	);

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


request_status_t get_device_location_from_bssid(request_device_location_payload_t *payload)
{
	esp_http_client_config_t config = {
		.url = "http://192.168.0.172:3000/location",
//	.url = "http://10.207.218.103:3000/location"
	};

	esp_http_client_handle_t client = esp_http_client_init(&config);

	esp_http_client_set_method(client, HTTP_METHOD_GET);
	esp_http_client_set_header(client, "Content-Type", "application/octet-stream");

	my_wifi_ap_record_t record = {
		.bssid = {0x64, 0xFA, 0x2B, 0x3A, 0x88, 0xD2},
		.rssi = -66
	};

	my_wifi_ap_record_t record2 = {
		.bssid = {0x8C, 0x9A, 0x8F, 0x08, 0x9B, 0x3E},
		.rssi = -67
	};

	my_wifi_ap_record_t record3 = {
		.bssid = {0x3C, 0x6A, 0xD2, 0xE9, 0x54, 0xD2},
		.rssi = -78
	};

	request_device_location_payload_t wire = {
		.number_aps = 3,
	};

	memcpy(&wire.aps[0], &record, sizeof(record));
	memcpy(&wire.aps[1], &record2, sizeof(record));
	memcpy(&wire.aps[2], &record3, sizeof(record));

	esp_http_client_set_post_field(
		client,
		(char *)&wire,
		sizeof(uint8_t) +  sizeof(my_wifi_ap_record_t)*3
	);

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
		16384,
		NULL,
		14,
		NULL,
		1
	);
			
	return REQUEST_SUCCESS;
}