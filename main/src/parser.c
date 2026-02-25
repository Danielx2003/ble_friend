#include "../include/parser.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>

static parser_action_table_t *g_actions;

parser_status_t parse_adv_data(const uint8_t* adv_data, size_t adv_data_len, parser_result_t* out_result)
{
  parser_status_t status;
	mfg_data_t mfg;

  status = extract_mfg_data(adv_data, adv_data_len, &mfg);
  if (status != PARSER_SUCCESS) {
    return status;
  }
	out_result->mfg = mfg;

  return parse_mfg_data(&(out_result->mfg), out_result);
}

parser_status_t extract_mfg_data(const uint8_t* adv_data, size_t adv_data_len, mfg_data_t* out)
{
  if (adv_data_len < MIN_ADV_DATA_LEN) {
    return PARSER_ERR_ADV_DATA_LEN;
  }

  int i = 0;

  while (i < adv_data_len) {
    uint8_t field_len = adv_data[i++];
    uint8_t type = adv_data[i++];

    if (i + field_len - 2 >= adv_data_len) {
      return PARSER_ERR_MFG_DATA;
    }

    if (type == 0xFF) {
      if (field_len < PROTOCOL_HEADER_SIZE) {
        return PARSER_ERR_PROTOCOL_HEADER;
      }

      out->company_id = adv_data[i] | (adv_data[i + 1] << 8);
      out->version_mode = adv_data[i + 2];
      out->flags = adv_data[i + 3];

      if (field_len <= PROTOCOL_HEADER_SIZE) {
        return PARSER_ERR_MFG_DATA;
      }

      memcpy(out->payload, &adv_data[i + 4], field_len - 5);
      out->payload_len = field_len - 5;

      return PARSER_SUCCESS;
    }

    i += field_len - 1;
  }

  return PARSER_ERR_MFG_DATA;
}

parser_status_t parse_mfg_data(mfg_data_t* mfg, parser_result_t* out_result)
{
  if (mfg->company_id != COMPANY_ID) {
    return PARSER_ERR_COMPANY_ID;
  }

  return parse_protocol_msg((mfg->version_mode & 0x0F), out_result);
}

parser_status_t parse_protocol_msg(parser_msg_t type, parser_result_t* out_result)
{
  switch (type) {
    case PARSER_PAIRING_MSG:
      out_result->action = g_actions->on_pairing;
      break;

    case PARSER_PAIRED_MSG:
      out_result->action = g_actions->on_paired;
      break;

    case PARSER_LOST_MSG:
      out_result->action = g_actions->on_lost;
      break;

    default:
      return PARSER_ERR_MODE;
  }

  return PARSER_SUCCESS;
}

void parser_init(parser_action_table_t* actions)
{
  g_actions = actions;
}