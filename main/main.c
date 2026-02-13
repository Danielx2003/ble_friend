#include "esp_log.h"
#include "nvs_flash.h"
/* BLE */
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "console/console.h"
#include "services/gap/ble_svc_gap.h"
#include "periodic_sync.h"
#include "host/ble_gap.h"
#include <stdbool.h>
#include <string.h>

#include "psa/crypto.h"

#define PROTOCOL_HEADER_MIN_SIZE 4

typedef struct {
    uint16_t company_id;
    uint8_t version_mode;
    uint8_t flags;
    uint8_t *public_key;
    size_t pubkey_len;
} mfg_data_t;


static const char* tag = "BLE_SCAN_APP"; 

void ble_store_config_init(void);

//BLE_HS_ADV_TYPE_MFG_DATA

static int gap_event_cb(struct ble_gap_event *event, void *arg)
{
	printf("Event Type: %d\n", event->type);
	if (event->type != 19)
	{
		printf("Event type NOT Extended Discovery");
	}
	
	switch(event->type)
	{
		case BLE_GAP_EVENT_EXT_DISC: {
			struct ble_gap_ext_disc_desc *disc = ((struct ble_gap_ext_disc_desc *)(&event->ext_disc));
			
			uint8_t data_len = disc->length_data;
			if (data_len >= 32)
			{
				ESP_LOGI(tag, "ADV Packet Length: %d\n", data_len);
				
			}
			break;
		}
	}
	
	return 0;
}

void start_scan(void)
{
	struct ble_gap_disc_params params;
	uint8_t own_addr_type;
	int rc;
	
	memset(&params, 0, sizeof(params));
	
	rc = ble_hs_id_infer_auto(0, &own_addr_type);
	if (rc != 0)
	{
		ESP_LOGE(tag, "Could not infer address type");
		return;
	}
	
	params.filter_duplicates = 1;
	params.passive = 1;
	
	rc = ble_gap_disc(own_addr_type, BLE_HS_FOREVER, &params, gap_event_cb, NULL);
	if (rc != 0)
	{
		ESP_LOGE(tag, "Failed to start discovery rc=%d\n", rc);
		return;
	}
	
	printf("Started Discovery\n");
}

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
	
	ESP_LOGI(tag, "Calling Start Scan");
	start_scan();
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