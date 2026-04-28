#pragma once

#include "host/ble_hs.h"

#include <stdint.h>
#include <stdbool.h>
#include "ble_types.h"
#include "ble_worker.h"
#include "mfg_data.h"

extern RTC_SLOW_ATTR struct timeval disc_start_time;

void handle_on_sync(void);
void handle_on_lost_msg();

extern int payloads_received;

/*Disc*/

#define PUB_KEY_SERVICE_UUID 0x87, 0xb0, 0x8f, 0x1d, 0xb3, 0x5f, 0xa5, 0xa1, 0x1d, 0x4c, 0xe3, 0x77, 0x76, 0x1b, 0xc4, 0xb0
#define PUB_KEY_CHAR_UUID 0x00, 0x1b, 0x15, 0x3a, 0xbc, 0x4c, 0xac, 0x94, 0xaf, 0x49, 0x32, 0xb9, 0x86, 0xd0, 0x60, 0x46
#define PEER_PUB_KEY_CHAR_WRITE_UUID 0x57, 0x4f, 0x5d, 0x9f, 0x44, 0x04, 0x19, 0xb5, 0xde, 0x41, 0x02, 0x26, 0x7f, 0xab, 0x0a, 0xfe

ble_status_t disc_set_event_handlers(ble_event_cbs_t* cbs);

int on_read(uint16_t conn_handle,
                   const struct ble_gatt_error *error,
                   struct ble_gatt_attr *attr,
                   void *arg);

/* Event Handlers */

/*
	Callback for BLE_WORKER_EVENT_CONNECT
*/
ble_status_t handle_on_connect(ble_work_item_t *msg);

/*
	Callback for BLE_WORKER_EVENT_DISCONNECT
*/
ble_status_t handle_on_disconnect(ble_work_disconnect_t *disconnect);

/*
	Callback for BLE_WORKER_EVENT_DISC_COMPLETE
*/
ble_status_t handle_disc_complete(ble_work_disc_complete_t *disc);

/*
	Callback for BLE_WORKER_EVENT_READ_COMPLETE
*/
ble_status_t handle_read_complete(ble_work_read_complete_t *read);

/*
	Callback for BLE_WORKER_EVENT_READ_COMPLETE
*/
ble_status_t handle_new_connection(ble_work_connect_t *connect);

/*
	Callback for BLE_WORKER_EVENT_ENC_CHANGE
*/
ble_status_t handle_enc_change(ble_work_connect_t *connect);

/*
	Callback for BLE_WORKER_EVENT_WRITE_KEY_TO_PEER
*/
ble_status_t write_key_to_peer(ble_work_write_key_t *item);


/*
	Pairing message handler
*/
void handle_pairing_msg(ble_work_msg_t *msg, mfg_data_t *mfg);

/*
	Paired message handler
*/
void handle_paired_msg(ble_work_msg_t *msg, mfg_data_t *mfg);

/*
	Lost message handler
*/
void handle_lost_msg(ble_work_msg_t *msg, mfg_data_t *mfg);


/* Public API */

/*
	Start BLE worker task
*/
ble_status_t ble_start(void);

/*
	Start discovery instance
*/
ble_status_t disc_start(ble_disc_params_t* params, uint32_t duration);

/*
	Stop discovery instance
*/
ble_status_t disc_stop();

/*
	Handle extended discovery packet
*/
ble_status_t handle_ext_disc(ble_work_item_t *msg);

/*
	Connect to peer device
*/
ble_status_t start_connect(ble_work_msg_t *msg);

/*
	Discover all peer services
*/
ble_status_t discover_all_services(ble_work_connect_t *connect);

