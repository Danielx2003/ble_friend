#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "ble_types.h"
#include "mfg_data.h"

#define PARSER_SUCCESS ((parser_status_t)0)
#define PARSER_ERR_COMPANY_ID ((parser_status_t)-1)
#define PARSER_ERR_VERSION ((parser_status_t)-2)
#define PARSER_ERR_PROTOCOL_HEADER ((parser_status_t)-3)
#define PARSER_ERR_ADV_DATA_LEN ((parser_status_t)-4)
#define PARSER_ERR_MFG_DATA ((parser_status_t)-5)
#define PARSER_ERR_MODE ((parser_status_t)-6)

#define PAYLOAD_MAX_SIZE 255
#define PROTOCOL_HEADER_SIZE 4
#define MIN_ADV_DATA_LEN 3

// Unsure
#define COMPANY_ID 0xFFFF

typedef void (*parser_action_fn)(ble_work_msg_t *msg, mfg_data_t *mfg);

typedef struct {
  parser_action_fn on_pairing;
  parser_action_fn on_paired;
  parser_action_fn on_lost;
} parser_action_table_t;

typedef int32_t parser_status_t;

typedef struct {
  parser_action_fn action;
  mfg_data_t *mfg;
} parser_result_t;


void parser_init(parser_action_table_t* actions);

parser_status_t parse_adv_data_fast(
    const uint8_t* adv_data,
    size_t adv_len,
    parser_result_t* out);
	
/*
	Parse raw advertising data payload
*/
parser_status_t parse_adv_data(const uint8_t* adv_data, size_t adv_data_len, parser_result_t* out_result);

/*
	Extract manufacturer data from advertising payload
*/
parser_status_t extract_mfg_data(const uint8_t* adv_data, size_t adv_data_len, mfg_data_t* out);

/*
	Validate manufacturer data
*/
parser_status_t parse_mfg_data(mfg_data_t* mfg, parser_result_t* out_result);

/*
	Invokes appropriate message handler for payload protocol
*/
parser_status_t parse_protocol_msg(parser_msg_t type, parser_result_t* out_result);
