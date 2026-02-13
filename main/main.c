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

static const char* tag = "BLE_SCAN_APP"; 

void ble_store_config_init(void);

//BLE_HS_ADV_TYPE_MFG_DATA

void scan_on_reset(int reason)
{
	
}

void scan_on_sync(void)
{
	ESP_LOGI(tag, "Host has Synced");
	int rc;
	rc = ble_hs_util_ensure_addr(0);
	if (rc != 0)
	{
		ESP_LOGE(tag, "No valid Address available. rc=%d\n", rc);
	}

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
	int rc;
	/* Initialize NVS — it is used to store PHY calibration data */
	esp_err_t ret = nvs_flash_init();
	if  (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
	    ESP_ERROR_CHECK(nvs_flash_erase());
	    ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

	ret = nimble_port_init();
	if (ret != ESP_OK) {
	    MODLOG_DFLT(ERROR, "Failed to init nimble %d \n", ret);
	    return;
	}

	/* Configure the host. */
	ble_hs_cfg.reset_cb = scan_on_reset;
	ble_hs_cfg.sync_cb = scan_on_sync;
	ble_hs_cfg.store_status_cb = ble_store_util_status_rr;
	
	ble_store_config_init();
	nimble_port_freertos_init(scan_host_task);
}