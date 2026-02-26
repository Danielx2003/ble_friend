#include "request.h"
#include "esp_log.h"
#include "esp_http_client.h"

static const char *TAG = "wifi station";

request_status_t upload_lost_details(request_lost_payload_t *payload)
{
	printf("calling upload\n");
	esp_http_client_config_t config = {
		.url = "http://192.168.0.172:3000/test",
	};
	esp_http_client_handle_t client = esp_http_client_init(&config);

	// GET Request
	esp_http_client_set_method(client, HTTP_METHOD_GET);
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