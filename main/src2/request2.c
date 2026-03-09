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


esp_err_t send_ecdsa_public_key_event_handler(esp_http_client_event_t *evt)
{
	request_ecdsa_response_t *response = (request_ecdsa_response_t *)evt->user_data;

  switch(evt->event_id) {
    case HTTP_EVENT_ON_DATA:
      ESP_LOGI(TAG, "Received %d bytes", evt->data_len);
      printf("%.*s\n", evt->data_len, (char*)evt->data);
			memcpy(response->uuid, evt->data, 36);
      break;

    default:
      break;
  }

  return ESP_OK;
}


request_status_t send_ecdsa_public_key(request_ecdsa_payload_t *payload, request_ecdsa_response_t *response)
{
	esp_http_client_config_t config = {
		.url = "http://192.168.0.172:3000/register",
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