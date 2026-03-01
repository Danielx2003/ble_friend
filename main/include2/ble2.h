#pragma once

#include "ble_worker2.h"

typedef enum {
	BLE_SUCCESS,
	BLE_ERR_NO_MEMORY,
	BLE_FAIL
} ble_status_t;

typedef struct {
	int reason;
} ble_event_reset_t;

typedef struct {
  void (*on_ready)(void);
  void (*on_reset)(ble_event_reset_t* reset);
} ble_callbacks_t;

void handle_on_sync(void);
void handle_on_lost_msg();


/*Disc*/

#include "parser2.h"
#include <stdint.h>
#include <stdbool.h>

#define PUB_KEY_SERVICE_UUID 0x87, 0xb0, 0x8f, 0x1d, 0xb3, 0x5f, 0xa5, 0xa1, 0x1d, 0x4c, 0xe3, 0x77, 0x76, 0x1b, 0xc4, 0xb0
#define PUB_KEY_CHAR_UUID 0x00, 0x1b, 0x15, 0x3a, 0xbc, 0x4c, 0xac, 0x94, 0xaf, 0x49, 0x32, 0xb9, 0x86, 0xd0, 0x60, 0x46
#define PEER_PUB_KEY_CHAR_WRITE_UUID 0x57, 0x4f, 0x5d, 0x9f, 0x44, 0x04, 0x19, 0xb5, 0xde, 0x41, 0x02, 0x26, 0x7f, 0xab, 0x0a, 0xfe

typedef enum {
	BLE_DISCONNECT_EVENT,
	BLE_CONNECT_EVENT,
	BLE_EXT_DISC_EVENT
} ble_event_t;

typedef struct {
	void (*on_connect)(void* ctx);
	void (*on_disconnect)(void* ctx);
	void (*on_ext_disc)(void* ctx);
} ble_event_cbs_t;

ble_status_t disc_set_event_handlers(ble_event_cbs_t* cbs);

typedef struct {
  uint8_t filter_duplicates:1;
  uint8_t passive:1;
  uint16_t interval;
} ble_disc_params_t;


/* Event Handlers */

ble_status_t handle_on_connect(ble_work_item_t *msg);
ble_status_t handle_on_disconnect();
ble_status_t handle_disc_complete(ble_work_disc_complete_t *disc);
ble_status_t handle_read_complete(ble_work_read_complete_t *read);
ble_status_t handle_new_connection(ble_work_connect_t *connect);
ble_status_t handle_enc_change(ble_work_connect_t *connect);


bool handle_pairing_msg(ble_work_msg_t *msg, mfg_data_t *mfg);
bool handle_paired_msg(ble_work_msg_t *msg, mfg_data_t *mfg);
bool handle_lost_msg(ble_work_msg_t *msg, mfg_data_t *mfg);
/* Public API */

/*
Initialise necessary callbacks, BLE services etc
*/
ble_status_t ble_init();

/*
Start the BLE Worker Task
*/
ble_status_t ble_start(void);

ble_status_t disc_start(ble_disc_params_t* params, uint32_t duration);
ble_status_t disc_stop();
ble_status_t handle_ext_disc(ble_work_item_t *msg);
ble_status_t start_connect(ble_work_msg_t *msg);
ble_status_t discover_all_services(ble_work_connect_t *connect);

