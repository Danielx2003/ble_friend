#pragma once

#include "parser.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#define BLE_WORK_QUEUE_LEN 5

extern QueueHandle_t ble_work_queue;

typedef enum {
    BLE_WORK_LOST_MSG,
		BLE_WORK_EXT_DISC
} ble_work_type_t;

typedef struct {
    ble_work_type_t type;
    void *ctx;
    mfg_data_t mfg;
} ble_work_item_t;

static void ble_worker_task(void *arg);