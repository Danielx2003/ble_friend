#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define PAYLOAD_MAX_SIZE 255

#define PROTOCOL_HEADER_SIZE 4
#define MIN_ADV_DATA_LEN 3
#define COMPANY_ID 0xFFFF

typedef enum {
    PARSER_PAIRING_MSG,
    PARSER_PAIRED_MSG,
    PARSER_LOST_MSG
} parser_msg_t;

typedef struct {
    uint16_t company_id;
    uint8_t  version_mode;
    uint8_t  flags;
    uint8_t  payload_len;
    uint8_t  payload[PAYLOAD_MAX_SIZE];
} mfg_data_t;