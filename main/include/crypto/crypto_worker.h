#pragma once

#include "freertos/FreeRTOS.h"

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "mfg_data.h"

#define CRYPTO_QUEUE_LEN 128

extern QueueHandle_t crypto_worker_queue;

typedef enum {
	CRYPTO_WORKER_EVENT_LOST_MSG,
	CRYPTO_WORKER_EVENT_READ_COMPLETE,
	CRYPTO_WORKER_DECRYPT_LOC_REPORT
} crypto_worker_event_t;

typedef struct {
    uint16_t conn_handle;
    int      status;
    uint8_t  data[32];
    uint16_t data_len;
} crypto_work_read_complete_t;

typedef struct {
    uint16_t conn_handle;
    int      status;
} crypto_work_disc_complete_t;

typedef struct {
    mfg_data_t mfg;
    int32_t location[2];
} crypto_work_lost_msg_t;

typedef struct {
	uint8_t enc_loc[24];
	uint8_t finder_key_raw[32];
	size_t finder_key_size;
} crypto_work_decrypt_loc_t;

typedef struct {
	crypto_worker_event_t type;
	union {
		crypto_work_lost_msg_t lost_msg;
		crypto_work_read_complete_t read_complete;
		crypto_work_decrypt_loc_t decrypt_loc;
	} context;
} crypto_work_item_t;

void crypto_worker_task(void *param);
