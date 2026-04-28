#pragma once

#include "mfg_data.h"
#include "freertos/FreeRTOS.h"

#define REQUEST_QUEUE_LEN 32

typedef enum {
	REQUEST_WORKER_EVENT_UPLOAD_LOST_LOCATION,
	REQUEST_WORKER_EVENT_FETCH_LOST_DEVICE_LOCATION,
	REQUEST_WORKER_EVENT_GET_LOCATION
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
	mfg_data_t mfg;
} request_user_location_t;

typedef struct {
	size_t len;
	uint8_t eph_pub_key[256];
} request_location_for_eph_key_t;

typedef struct {
	request_worker_event_t type;
	union {
		request_lost_payload_t lost_payload;
		request_location_for_eph_key_t get_device_loc;
		request_user_location_t get_user_loc;
	};
} request_work_item_t;

extern QueueHandle_t request_worker_queue;

void request_worker_task(void *param);
