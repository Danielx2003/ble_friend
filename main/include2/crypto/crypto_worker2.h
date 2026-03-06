#pragma once

#include "../crypto/crypto_worker2.h"

#include "parser2.h"

#define CRYPTO_QUEUE_LEN 128

extern QueueHandle_t crypto_worker_queue;

typedef enum {
	CRYPTO_WORKER_EVENT_LOST_MSG,
	CRYPTO_WORKER_EVENT_READ_COMPLETE
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
} crypto_work_lost_msg_t;

typedef struct {
	crypto_worker_event_t type;
	union {
		crypto_work_lost_msg_t lost_msg;
		crypto_work_read_complete_t read_complete;
	} context;
} crypto_work_item_t;

void crypto_worker_task(void *param);
