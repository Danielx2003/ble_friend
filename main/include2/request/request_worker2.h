#pragma once

#include "freertos/FreeRTOS.h"
#include "crypto2.h"

#define REQUEST_QUEUE_LEN 32

typedef enum {
	REQUEST_WORKER_EVENT_UPLOAD_LOST_LOCATION
} request_worker_event_t;

typedef struct {
	char device_id[36];
	size_t encryption_location_len;
	uint8_t encrypted_location[128];
	uint8_t finder_key_raw[32];
	uint8_t lost_eph_pub_key_raw[32];
	uint8_t signature[64];
} request_lost_payload_t;

typedef struct {
	request_worker_event_t type;
	union {
		request_lost_payload_t lost_payload;
	};
} request_work_item_t;

extern QueueHandle_t request_worker_queue;

void request_worker_task(void *param);
