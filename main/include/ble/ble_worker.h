#pragma once

#include "freertos/FreeRTOS.h"

#define BLE_QUEUE_LEN 128

extern QueueHandle_t ble_worker_queue;

void ble_worker_task(void *param);

