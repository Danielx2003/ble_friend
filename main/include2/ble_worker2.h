#pragma once

#include "freertos/FreeRTOS.h"

#define BLE_QUEUE_LEN 16

extern QueueHandle_t ble_worker_queue;

typedef enum {
	BLE_WORKER_EVENT_CONNECT,
	BLE_WORKER_EVENT_DISCONNECT,
	BLE_WORKER_EVENT_EXT_DISCOvery,
	BLE_WORKER_EVENT_HOST_SYNC,
	BLE_WORKER_EVENT_HOST_RESET
} ble_worker_event_t;

typedef struct {
	ble_worker_event_t type;
} ble_work_item_t;

void ble_worker_task(void *param);