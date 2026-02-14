#include "esp_log.h"
#include "nvs_flash.h"
/* BLE */
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "console/console.h"
#include "services/gap/ble_svc_gap.h"
#include "host/ble_gap.h"
#include <stdbool.h>
#include <string.h>

#include "psa/crypto.h"

#include "disc.h"
#include "ble.h"

static const char* tag = "BLE_SCAN_APP"; 

void ble_store_config_init(void);

//BLE_HS_ADV_TYPE_MFG_DATA

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

void scan_host_task(void* param)
{
	ESP_LOGI(tag, "BLE Host Task Started");
	/* This function will return only when nimble_port_stop() is executed */
	nimble_port_run();

	nimble_port_freertos_deinit();
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