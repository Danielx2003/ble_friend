#pragma once

#include <stdint.h>
#include <stddef.h>

/* BLE */

typedef enum {
	BLE_SUCCESS,
	BLE_ERR_NO_MEMORY,
	BLE_ERR_MTU_EXCHANGE,
	BLE_ERR_UPGRADE_CONN,
	BLE_FAIL
} ble_status_t;

typedef struct {
	int reason;
} ble_event_reset_t;

typedef struct {
  void (*on_ready)(void);
  void (*on_reset)(ble_event_reset_t* reset);
} ble_callbacks_t;

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

typedef struct {
  uint8_t filter_duplicates:1;
  uint8_t passive:1;
  uint16_t interval;
} ble_disc_params_t;

/* BLE Worker */

typedef struct {
  uint8_t type;
  uint8_t val[6];
} ble_addr_local_t;

typedef enum {
	BLE_WORKER_EVENT_CONNECT,
	BLE_WORKER_EVENT_DISCONNECT,
	BLE_WORKER_EVENT_EXT_DISC,
	BLE_WORKER_EVENT_HOST_SYNC,
	BLE_WORKER_EVENT_HOST_RESET,
	BLE_WORKER_EVENT_DISC_COMPLETE,
	BLE_WORKER_EVENT_READ_COMPLETE,
	BLE_WORKER_EVENT_WRITE_KEY_TO_PEER,
	BLE_WORKER_EVENT_ENC_CHANGE
} ble_worker_event_t;

typedef struct {
	uint16_t conn_handle;
	uint8_t  pub_key[32];
	size_t pub_key_len;
} ble_work_write_key_t;

typedef struct {
	ble_addr_local_t addr;
} ble_work_pairing_msg_t;

typedef struct {
} ble_work_paired_msg_t;

typedef struct {
} ble_work_lost_msg_t;

typedef struct {
	uint16_t conn_handle;
} ble_work_connect_t;

typedef struct {
	uint16_t conn_handle;
} ble_work_disconnect_t;

typedef struct {
    uint16_t conn_handle;
    int      status;
    uint8_t  data[32];
    uint16_t data_len;
} ble_work_read_complete_t;

typedef struct {
    uint16_t conn_handle;
    int      status;
} ble_work_disc_complete_t;

typedef struct {
	uint8_t data[255];
	uint8_t len;
	ble_work_pairing_msg_t pairing;
	ble_work_paired_msg_t paired;
	ble_work_lost_msg_t lost;
} ble_work_msg_t;

typedef struct {
	ble_worker_event_t type;
	union {
		ble_work_msg_t msg;
		ble_work_connect_t connect;
		ble_work_disconnect_t disconnect;
		ble_work_read_complete_t read_complete;
		ble_work_disc_complete_t disc_complete;
		ble_work_write_key_t write_pub_key;
	} context;
} ble_work_item_t;