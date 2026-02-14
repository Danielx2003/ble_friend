#include "esp_log.h"

#include "disc.h"
#include "ble.h"

#include <stdbool.h>

static const char* tag = "BLE_SCAN_APP"; 

void ble_store_config_init(void);

void on_reset(ble_event_reset_t* reset)
{
	printf("Reset Reason: %d", reset->reason);
}

void on_sync(void)
{
	ESP_LOGI(tag, "Host has Synced");

	ble_disc_params_t params = {
		.filter_duplicates = 1,
		.passive = 1,
	};
	
	ble_status_t status = disc_start(&params, 0);
	printf("Status: %d\n", status);
}

void app_main(void)
{
	device_init();

	ble_callbacks_t callbacks = {
	    .on_ready = on_sync,
	    .on_reset = on_reset
	};
	
	ble_init(&callbacks);
	ble_start();
}