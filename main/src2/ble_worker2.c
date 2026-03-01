#include "ble2.h"
#include "ble_worker2.h"

#include "freertos/idf_additions.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "esp_central.h"

void ble_store_config_init(void);

/* NimBLE Callbacks */

void nimble_on_reset() {}

void nimble_on_sync()
{
	if (ble_worker_queue == NULL) {
	    return;
	}
	
	ble_work_item_t item = {
		.type = BLE_WORKER_EVENT_HOST_SYNC
	};
	xQueueSend(ble_worker_queue, &item, 0);
}

/* NimBLE Task */

void my_ble_host_task(void* param)
{
  nimble_port_run();
  nimble_port_freertos_deinit();
}

/* BLE Worker Task */

void ble_worker_task(void *param)
{
  nimble_port_init();

  ble_hs_cfg.reset_cb = nimble_on_reset;
  ble_hs_cfg.sync_cb  = nimble_on_sync;
  ble_hs_cfg.store_status_cb = ble_store_util_status_rr;
	
	ble_hs_cfg.sm_bonding = 0;
	ble_hs_cfg.sm_sc = 1;
	ble_hs_cfg.sm_our_key_dist = 1;
	ble_hs_cfg.sm_their_key_dist = 1;

  peer_init(4, 64, 64, 64);
  ble_att_set_preferred_mtu(256);
  ble_store_config_init();
	
  nimble_port_freertos_init(my_ble_host_task);

  ble_work_item_t item;

  while(1) {
	  if(xQueueReceive(ble_worker_queue, &item, portMAX_DELAY)) {
	    switch(item.type) {
	      case BLE_WORKER_EVENT_HOST_SYNC:
					handle_on_sync();
	        break;
				case BLE_WORKER_EVENT_EXT_DISC:
					handle_ext_disc(&item);
					break;
				case BLE_WORKER_EVENT_CONNECT:
					handle_on_connect(&(item));
					break;
				case BLE_WORKER_EVENT_DISCONNECT:
					handle_on_disconnect();	
				break;
				case BLE_WORKER_EVENT_DISC_COMPLETE:
			    handle_disc_complete(&item.context.disc_complete);
			    break;
				case BLE_WORKER_EVENT_READ_COMPLETE:
			    handle_read_complete(&item.context.read_complete);
			    break;
				case BLE_WORKER_EVENT_ENC_CHANGE:
					handle_enc_change(&item.context.connect);
	      default:
	        break;
	      }
	    }
  }
}