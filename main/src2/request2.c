#include "request2.h"
#include "crypto2.h"
#include "freertos/idf_additions.h"
#include "request_worker2.h"
#include "ble2.h"

#include "esp_log.h"
#include "esp_http_client.h"

/* Static + Global Variables */

static const char *TAG = "wifi station";

QueueHandle_t request_worker_queue = NULL;
char device_uuid[36];

/* Request Functions */

request_status_t upload_lost_batch(request_lost_payload_t *batch, size_t batch_len)
{
	return REQUEST_SUCCESS;
	
	/* Temp logic to generate finders keypair */
	crypto_key_t finder_priv_key;
	crypto_status_t status = generate_keypair(CRYPTO_CURVE_X25519, &finder_priv_key);
	if (status != CRYPTO_SUCCESS)
	{
		printf("failed to generate keypair\n");
		return REQUEST_ERR_UNKNOWN;
	}

	crypto_key_t finder_public_key;
	status = export_public_key(&finder_priv_key, &finder_public_key, 32);
	if (status != CRYPTO_SUCCESS)
	{
		printf("failed to export publoc key\n");
		return REQUEST_ERR_UNKNOWN;
	}
	
	printf("uploading %d payloads\n", batch_len);
	esp_http_client_config_t config = {
//		.url = "http://192.168.0.172:3000/send",
		.url = "http://10.207.208.255:3000/send"
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
	
	printf("done\n");

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

//request_status_t upload_lost_details(request_lost_payload_t *payload)
//{
//	esp_http_client_config_t config = {
//		.url = "http://192.168.0.172:3000/send",
//	};
//	esp_http_client_handle_t client = esp_http_client_init(&config);
//
//	esp_http_client_set_method(client, HTTP_METHOD_POST);
//	esp_http_client_set_header(client, "Content-Type", "application/octet-stream");
//	
//	request_lost_wire_t wire_payload;
//
//	memcpy(wire_payload.device_id, payload->device_id, 36);
//	memcpy(wire_payload.location, payload->encrypted_location, 128);
//	memcpy(wire_payload.public_key, payload->finder_key->raw.data, 32);
////	wire_payload.public_key_len = payload->finder_key->raw.len;
//	memcpy(wire_payload.signature, payload->signature, 64);
//
//	esp_http_client_set_post_field(
//		client,
//		(char *)&wire_payload,
//		sizeof(wire_payload)
//	);
//
//	esp_err_t err = esp_http_client_perform(client);
//	
//	if (err == ESP_OK) {
//	    ESP_LOGI(TAG, "HTTPS Status = %d, content_length = %"PRId64,
//	            esp_http_client_get_status_code(client),
//	            esp_http_client_get_content_length(client));
//	} else {
//	    ESP_LOGE(TAG, "Error perform http request %s", esp_err_to_name(err));
//	}
//	esp_http_client_cleanup(client);
//
//  return REQUEST_SUCCESS;
//}


esp_err_t send_ecdsa_public_key_event_handler(esp_http_client_event_t *evt)
{
//	request_ecdsa_response_t *response = (request_ecdsa_response_t *)evt->user_data;

  switch(evt->event_id) {
    case HTTP_EVENT_ON_DATA:
      printf("Received %d bytes", evt->data_len);
      printf("%.*s\n", evt->data_len, (char*)evt->data);
			memcpy(device_uuid, evt->data, evt->data_len);
      break;

    default:
      break;
  }

  return ESP_OK;
}


request_status_t send_ecdsa_public_key(request_ecdsa_payload_t *payload, request_ecdsa_response_t *response)
{
	return REQUEST_SUCCESS;
	
	esp_http_client_config_t config = {
//		.url = "http://192.168.0.172:3000/register",
		.url = "http://10.207.208.255:3000/register",
		.event_handler = send_ecdsa_public_key_event_handler,
		.user_data = response
	};

	esp_http_client_handle_t client = esp_http_client_init(&config);

	esp_http_client_set_method(client, HTTP_METHOD_POST);

	crypto_key_t public_key;
	crypto_status_t status = export_ecdsa_public_key(
		payload->ecdsa_public_key,
		&public_key
	);
	if (status != CRYPTO_SUCCESS) { return REQUEST_ERR_UNKNOWN; }

	esp_http_client_set_header(client, "Content-Type", "application/octet-stream");

	esp_http_client_set_post_field(
	    client,
	    (char*)public_key.raw.data,
	    public_key.raw.len
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
	printf("Get location \n");
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