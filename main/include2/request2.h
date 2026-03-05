#pragma once

#include "crypto2.h"
#include "request_worker2.h"

typedef enum {
	REQUEST_SUCCESS,
	REQUEST_ERR_INIT,
	REQUEST_ERR_NO_MEMORY
} request_status_t;

typedef struct {
	crypto_key_t *finder_key;
	uint8_t location; // Filler until we decide how we get location
} request_lost_payload_t;

/* Public API */

request_status_t upload_batch(request_work_item_t *batch, size_t batch_len);

/*
POST
Upload finders public key, encrypted lost message
*/
request_status_t upload_lost_details(request_lost_payload_t *payload);

/*
GET
Owner requests all location's with the given public key
Then he can decrypt - which can be done elsewhere(?)
*/
request_status_t get_all_locations();

/*
Start Request Task/Qeueue
*/
request_status_t request_init();
