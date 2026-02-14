#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

typedef enum {
  PARSER_PAIRING_MSG,
  PARSER_PAIRED_MSG,
  PARSER_LOST_MSG
} parser_msg_t;

typedef struct {
  uint16_t company_id;
  uint8_t version_mode;
  uint8_t flags;
  uint8_t payload_len;
  uint8_t* payload;
} mfg_data_t;

typedef int32_t parser_status_t;

#define PARSER_SUCCESS ((parser_status_t)0)
#define PARSER_ERR_COMPANY_ID ((parser_status_t)-1)
#define PARSER_ERR_VERSION ((parser_status_t)-2)
#define PARSER_ERR_PROTOCOL_HEADER ((parser_status_t)-3)
#define PARSER_ERR_ADV_DATA_LEN ((parser_status_t)-4)
#define PARSER_ERR_MFG_DATA ((parser_status_t)-5)
#define PARSER_ERR_MODE ((parser_status_t)-6)

// Protocol
#define PROTOCOL_HEADER_SIZE 4

// Adv Data
#define MIN_ADV_DATA_LEN 3

// Unsure
#define COMPANY_ID 0xFFFF

// parser_status_t parse(parser_msg_t type, uint8_t* data);
parser_status_t parse_adv_data(const uint8_t* adv_data, size_t adv_data_len);
parser_status_t extract_mfg_data(const uint8_t* adv_data, size_t adv_data_len, mfg_data_t* out);
parser_status_t parse_mfg_data(mfg_data_t* mfg);
parser_status_t parse_protocol_msg(parser_msg_t type, mfg_data_t* mfg);

// Message Handlers
parser_status_t handle_lost_msg(mfg_data_t* mfg);
parser_status_t handle_paired_msg(mfg_data_t* mfg);
parser_status_t handle_pairing_msg(mfg_data_t* mfg);
