#pragma once

#include "request_worker2.h"

#define MAX_BATCH_ITEMS 32

typedef enum {
	REQUEST_SUCCESS,
	REQUEST_ERR_INIT,
	REQUEST_ERR_NO_MEMORY,
	REQUEST_ERR_UNKNOWN
} request_status_t;

typedef struct {
	size_t size;
	request_lost_payload_t payloads[MAX_BATCH_ITEMS];
} request_lost_batch_wire_t;

typedef struct {
	char device_id[36];
	uint8_t location[2];
	uint8_t signature[64];
	uint8_t public_key[32];
//	size_t public_key_len;
} request_lost_wire_t;

typedef struct {
	uint8_t bssid[6];
	int8_t rssi;
} my_wifi_ap_record_t;

typedef struct {
	uint8_t number_aps;
	my_wifi_ap_record_t aps[3];
} request_device_location_payload_t;

/* Public API */

request_status_t upload_lost_batch(request_lost_payload_t *batch, size_t batch_len);

/*
POST
Upload finders public key, encrypted lost message
*/
request_status_t upload_lost_details(request_lost_payload_t *payload);

request_status_t get_device_location_from_bssid(request_device_location_payload_t *payload);

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
