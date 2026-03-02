#pragma once

#include "freertos/FreeRTOS.h"

#define REQUEST_QUEUE_LEN 32

typedef enum {
	REQUEST_WORKER_EVENT_UPLOAD_LOST_LOCATION
} request_worker_event_t;

typedef struct {
	uint8_t pub_key[32];
	uint8_t location;
} request_work_upload_lost_loc_t;

typedef struct {
	request_worker_event_t type;
	union {
		request_work_upload_lost_loc_t lost_loc;
	};
} request_work_item_t;

extern QueueHandle_t request_worker_queue;

void request_worker_task(void *param);