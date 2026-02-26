#pragma once

#include "crypto.h"

typedef enum {
	REQUEST_SUCCESS,
	REQUEST_ERR_INIT
} request_status_t;

typedef struct {
	crypto_key_t *finder_key;
	uint8_t location; // Filler until we decide how we get location
} request_lost_payload_t;


/* Public API */

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

