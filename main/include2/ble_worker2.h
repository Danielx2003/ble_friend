#pragma once

#include "freertos/FreeRTOS.h"
#include "host/ble_hs.h"
#include "nimble/ble.h"

#define BLE_QUEUE_LEN 16

extern QueueHandle_t ble_worker_queue;

typedef enum {
	BLE_WORKER_EVENT_CONNECT,
	BLE_WORKER_EVENT_DISCONNECT,
	BLE_WORKER_EVENT_EXT_DISC,
	BLE_WORKER_EVENT_HOST_SYNC,
	BLE_WORKER_EVENT_HOST_RESET,
	BLE_WORKER_EVENT_DISC_COMPLETE,
	BLE_WORKER_EVENT_READ_COMPLETE,
	BLE_WORKER_EVENT_ENC_CHANGE
} ble_worker_event_t;

typedef struct {
	ble_addr_t addr;
} ble_work_pairing_msg_t;

typedef struct {
} ble_work_paired_msg_t;

typedef struct {
} ble_work_lost_msg_t;

typedef struct {
	uint16_t conn_handle;
} ble_work_connect_t;

typedef struct {
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
	} context;
} ble_work_item_t;


void ble_worker_task(void *param);

