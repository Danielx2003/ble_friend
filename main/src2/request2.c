#include "request2.h"
#include "crypto2.h"
#include "crypto_worker2.h"
#include "freertos/idf_additions.h"
#include "parser2.h"
#include "request_worker2.h"

#include "esp_log.h"
#include "esp_http_client.h"

/* Static + Global Variables */

static const char *tag = "wifi station";

QueueHandle_t request_worker_queue = NULL;
char device_uuid[36];

/* Request Functions */

request_status_t upload_lost_batch(request_lost_payload_t *batch, size_t batch_len)
{
	esp_http_client_config_t config = {
		.url = "http://192.168.1.196:3000/send",
	};
	esp_http_client_handle_t client = esp_http_client_init(&config);

	esp_http_client_set_method(client, HTTP_METHOD_POST);
	esp_http_client_set_header(client, "Content-Type", "application/octet-stream");
	
	request_lost_batch_wire_t data = {
		.size = 1,
	};
	
	memcpy(data.payloads, batch, sizeof(request_lost_payload_t) * 1);

	
	esp_http_client_set_post_field(
		client,
		(char *)&data,
		sizeof(size_t) + sizeof(request_lost_payload_t) * 1 // 1 was batch_len
	);

	esp_err_t err = esp_http_client_perform(client);

	if (err == ESP_OK) {
	    ESP_LOGI(tag, "HTTPS Status = %d, content_length = %"PRId64,
	            esp_http_client_get_status_code(client),
	            esp_http_client_get_content_length(client));
	} else {
	    ESP_LOGE(tag, "Error perform http request %s", esp_err_to_name(err));
	}
	esp_http_client_cleanup(client);

	return REQUEST_SUCCESS;
}


esp_err_t send_ecdsa_public_key_event_handler(esp_http_client_event_t *evt)
{
//	request_ecdsa_response_t *response = (request_ecdsa_response_t *)evt->user_data;

  switch(evt->event_id) {
    case HTTP_EVENT_ON_DATA:
      ESP_LOGE(tag, "Received %d bytes", evt->data_len);
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
	esp_http_client_config_t config = {
		.url = "http://192.168.1.196:3000/register",
//		.url = "http://10.207.208.255:3000/register",
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
	    ESP_LOGI(tag, "HTTPS Status = %d, content_length = %"PRId64,
	            esp_http_client_get_status_code(client),
	            esp_http_client_get_content_length(client));
	} else {
	    ESP_LOGE(tag, "Error perform http request %s", esp_err_to_name(err));
	}

	esp_http_client_cleanup(client);

	return REQUEST_SUCCESS;
}

esp_err_t get_lost_device_locations_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id) {
      case HTTP_EVENT_ON_DATA: {
        uint8_t *ptr = (uint8_t *)evt->data;

        // Read loc (length-prefixed)
        uint16_t enc_len = (ptr[0] << 8) | ptr[1];  ptr += 2;
        uint8_t *enc_location = ptr;                  ptr += enc_len;

        // Read finder key (length-prefixed)
        uint16_t key_len = (ptr[0] << 8) | ptr[1];  ptr += 2;
        uint8_t *finder_key = ptr;
				
				crypto_work_item_t item = {
					.type = CRYPTO_WORKER_DECRYPT_LOC_REPORT
				};
				memcpy(item.context.decrypt_loc.enc_loc, enc_location, 24);
				memcpy(item.context.decrypt_loc.finder_key_raw, finder_key, 32);
				xQueueSend(crypto_worker_queue, &item, 0);

        break;
      }

      default:
          break;
    }

    return ESP_OK;
}


request_status_t get_all_locations(request_location_for_eph_key_t *item)
{		
		esp_http_client_config_t config = {
			.url = "http://192.168.1.196:3000/fetch_device_locations",
			.event_handler = get_lost_device_locations_event_handler,
		};

		esp_http_client_handle_t client = esp_http_client_init(&config);
		esp_http_client_set_method(client, HTTP_METHOD_POST);
		esp_http_client_set_header(client, "Content-Type", "application/octet-stream");
		esp_http_client_set_post_field(
		    client,
		    (char*)item->eph_pub_key,
		    item->len
		);

		esp_err_t err = esp_http_client_perform(client);

		if (err == ESP_OK) {
		    ESP_LOGI(tag, "HTTPS Status = %d, content_length = %"PRId64,
		            esp_http_client_get_status_code(client),
		            esp_http_client_get_content_length(client));
		} else {
		    ESP_LOGE(tag, "Error perform http request %s", esp_err_to_name(err));
		}

		esp_http_client_cleanup(client);

		return REQUEST_SUCCESS;
}

#include "cJSON.h"

esp_err_t get_device_location_event_handler(esp_http_client_event_t *evt)
{
  switch(evt->event_id) {
    case HTTP_EVENT_ON_DATA:
      char *data = malloc(evt->data_len + 1);
      memcpy(data, evt->data, evt->data_len);
      data[evt->data_len] = '\0';

			crypto_work_item_t item = {
				.type = CRYPTO_WORKER_EVENT_LOST_MSG
			};
			memcpy(&item.context.lost_msg.mfg, evt->user_data, sizeof(mfg_data_t));
			item.context.lost_msg.mfg.payload_len = 32;
			
      cJSON *json = cJSON_Parse(data);
      if (json != NULL) {
        cJSON *lat = cJSON_GetObjectItem(json, "lat");
        cJSON *lng = cJSON_GetObjectItem(json, "lng");

        if (cJSON_IsNumber(lat) && cJSON_IsNumber(lng)) {
            printf("Lat: %f, Lng: %f\n", lat->valuedouble, lng->valuedouble);
						item.context.lost_msg.location[0] = (int32_t)(lat->valuedouble * 1e6);
						item.context.lost_msg.location[1] = (int32_t)(lng->valuedouble * 1e6);
						
						xQueueSend(crypto_worker_queue, &item, 0);
        }
        cJSON_Delete(json);
      }

      free(data);
      break;

    default:
        break;
  }

  return ESP_OK;
}

#include "esp_wifi.h"

request_status_t get_user_location(request_user_location_t *payload)
{
	esp_http_client_config_t config = {
		.url = "http://192.168.1.196:3000/location",
		.event_handler = get_device_location_event_handler,
		.user_data = &payload->mfg
		// pass mfg as user_data, then we can send it to the crypto event
	};

	esp_http_client_handle_t client = esp_http_client_init(&config);

	esp_http_client_set_method(client, HTTP_METHOD_GET);
	esp_http_client_set_header(client, "Content-Type", "application/octet-stream");
	
	
	wifi_scan_config_t scan_config = {
	    .ssid = NULL,
	    .bssid = NULL,
	    .channel = 0,
	    .scan_type = WIFI_SCAN_TYPE_ACTIVE,
	};

	esp_wifi_scan_start(&scan_config, true);

	uint16_t num_networks = 0;
	esp_wifi_scan_get_ap_num(&num_networks);
	
	wifi_ap_record_t* networks = (wifi_ap_record_t*)malloc(sizeof(wifi_ap_record_t) * num_networks);
	esp_wifi_scan_get_ap_records(&num_networks, networks);
	
	if (num_networks < 3) {
	    printf("Not enough networks found for geolocation\n");
	    free(networks);
	    return REQUEST_ERR_UNKNOWN;
	}
	
	my_wifi_ap_record_t record, record2, record3;
	my_wifi_ap_record_t *records[] = {&record, &record2, &record3};

	for (int i = 0; i < 3; i++) {
	    memcpy(records[i]->bssid, networks[i].bssid, 6);
	    records[i]->rssi = networks[i].rssi;
	}

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
	    ESP_LOGI(tag, "HTTPS Status = %d, content_length = %"PRId64,
	            esp_http_client_get_status_code(client),
	            esp_http_client_get_content_length(client));
	} else {
	    ESP_LOGE(tag, "Error perform http request %s", esp_err_to_name(err));
	}
	esp_http_client_cleanup(client);
	
	free(networks);

	return REQUEST_SUCCESS;
}

request_status_t request_init()
{
	request_worker_queue =
	    xQueueCreate(REQUEST_QUEUE_LEN, sizeof(request_work_item_t));

	if (!request_worker_queue) {
	  ESP_LOGE(tag, "Failed to create Request worker queue");
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