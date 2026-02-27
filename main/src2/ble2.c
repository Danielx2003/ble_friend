#include "ble2.h"

#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "esp_central.h"

#include <stdio.h>

static ble_callbacks_t app_callbacks;

void ble_store_config_init(void);

static const char *tag = "BLE_2";


/* Callbacks */

//static void nimble_on_sync(void)
//{
//    // Post event to BLE queue
//    ble_work_item_t item = {
//        .type = BLE_WORKER_EVENT_HOST_SYNC
//    };
//
//    xQueueSendFromISR(ble_worker_queue, &item, NULL);
//}
//
//static void nimble_on_reset(int reason)
//{
//    ble_work_item_t item = {
//        .type = BLE_WORKER_EVENT_HOST_RESET,
//    };
//
//    xQueueSendFromISR(ble_worker_queue, &item, NULL);
//}

/* Private Functions */

//void ble_host_task(void* param)
//{
//  nimble_port_run();
//  nimble_port_freertos_deinit();
//}

/* Public API */

ble_status_t ble_init()
{
//	esp_err_t ret;
//
//	ret = nimble_port_init();
//	if (ret != 0) {
//	    return -1;
//	}
//
//	ble_hs_cfg.reset_cb = nimble_on_reset;
//	ble_hs_cfg.sync_cb  = nimble_on_sync;
//	ble_hs_cfg.store_status_cb = ble_store_util_status_rr;
//
//	peer_init(4, 64, 64, 64);
//	ble_att_set_preferred_mtu(256);
//
//	ble_store_config_init();
	
	return BLE_SUCCESS;
}

QueueHandle_t ble_worker_queue = NULL;

ble_status_t ble_start()
{
	ble_worker_queue = xQueueCreate(BLE_QUEUE_LEN, sizeof(ble_work_item_t));
	if (!ble_worker_queue) {
	    ESP_LOGE(tag, "Failed to create BLE worker queue");
	    return BLE_ERR_NO_MEMORY;
	}
	
	xTaskCreate(
	    ble_worker_task,
	    "ble_worker",
	    8192,
	    NULL,
	    5,
	    NULL
	);
	
//	nimble_port_freertos_init(ble_host_task);

	return BLE_SUCCESS;
}