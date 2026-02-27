#include "request.h"
#include "esp_log.h"
#include "esp_http_client.h"

static const char *TAG = "wifi station";

request_status_t upload_lost_details(request_lost_payload_t *payload)
{		
	esp_http_client_config_t config = {
		.url = "http://192.168.0.172:3000/send",
	};
	esp_http_client_handle_t client = esp_http_client_init(&config);

	esp_http_client_set_method(client, HTTP_METHOD_POST);
	esp_http_client_set_header(client, "Content-Type", "application/json");
	
	uint8_t data[128];
	char hex[65];

	for (int i = 0; i < 32; i++) {
	    sprintf(&hex[i * 2], "%02X",
	            payload->finder_key->raw.data[i]);
	}
	hex[64] = '\0';

	int len = snprintf((char *)data, sizeof(data),
	                   "{\"key\":\"%s\"}",
	                   hex);

	if (len < 0 || len >= sizeof(data)) {
	    return -1;
	}

	esp_http_client_set_post_field(client, (char *)data, len);
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