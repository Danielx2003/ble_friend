#pragma once

#include "request_worker2.h"
#include "crypto2.h"

typedef enum {
	REQUEST_SUCCESS,
	REQUEST_ERR_INIT,
	REQUEST_ERR_NO_MEMORY,
	REQUEST_ERR_UNKNOWN
} request_status_t;

typedef struct {
	char device_id[36];
	uint8_t location[2]; // Filler until we decide how we get location
	crypto_key_t *finder_key; // Public key that encrypted the report
	uint8_t signature[64];
} request_lost_payload_t;

typedef struct {
	char device_id[36];
	uint8_t location[2];
	uint8_t signature[64];
	uint8_t public_key[32];
//	size_t public_key_len;
} request_lost_wire_t;

typedef struct {
	crypto_key_t *ecdsa_public_key;
} request_ecdsa_payload_t;

typedef struct {
	char uuid[36];
} request_ecdsa_response_t;

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

request_status_t send_ecdsa_public_key(
	request_ecdsa_payload_t *payload,
	request_ecdsa_response_t *response);

/*
Start Request Task/Qeueue
*/
request_status_t request_init();
