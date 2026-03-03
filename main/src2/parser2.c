#include "parser2.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>

static parser_action_table_t *g_actions;

parser_status_t parse_adv_data_fast(
    const uint8_t* adv_data,
    size_t adv_len,
    parser_result_t* out)
{
    if (adv_len < 2) {
        return PARSER_ERR_ADV_DATA_LEN;
    }

    size_t i = 0;

    while (i + 1 < adv_len) {

        uint8_t field_len = adv_data[i];
        if (field_len == 0) break;

        if (i + field_len >= adv_len) {
            return PARSER_ERR_MFG_DATA;
        }

        uint8_t type = adv_data[i + 1];

        if (type == 0xFF) {  // Manufacturer data

            if (field_len < PROTOCOL_HEADER_SIZE) {
                return PARSER_ERR_PROTOCOL_HEADER;
            }

            const uint8_t* field = &adv_data[i + 2];

            uint16_t company_id = field[0] | (field[1] << 8);

            // 🔥 FAST REJECT
            if (company_id != COMPANY_ID) {
                return PARSER_ERR_COMPANY_ID;
            }

            uint8_t version_mode = field[2];
            uint8_t flags = field[3];

            uint8_t msg_type = version_mode & 0x0F;

            // Assign callback immediately (no extra switch function)
            switch (msg_type) {
                case PARSER_PAIRING_MSG:
                    if (!g_actions->on_pairing) return PARSER_ERR_MODE;
                    out->action = g_actions->on_pairing;
                    break;

                case PARSER_PAIRED_MSG:
                    if (!g_actions->on_paired) return PARSER_ERR_MODE;
                    out->action = g_actions->on_paired;
                    break;

                case PARSER_LOST_MSG:
                    if (!g_actions->on_lost) return PARSER_ERR_MODE;
                    out->action = g_actions->on_lost;
                    break;

                default:
                    return PARSER_ERR_MODE;
            }

            // 🔥 Only copy payload AFTER validation
            size_t payload_len = field_len - 5;
            if (payload_len > 0) {
                memcpy(out->mfg->payload, &field[4], payload_len);
                out->mfg->payload_len = payload_len;
            }

            out->mfg->company_id = company_id;
            out->mfg->version_mode = version_mode;
            out->mfg->flags = flags;

            return PARSER_SUCCESS;
        }

        i += field_len + 1;
    }

    return PARSER_ERR_MFG_DATA;
}


parser_status_t parse_adv_data(const uint8_t* adv_data, size_t adv_data_len, parser_result_t *out_result)
{
  parser_status_t status;

  status = extract_mfg_data(adv_data, adv_data_len, out_result->mfg);
  if (status != PARSER_SUCCESS) {
    return status;
  }

  return parse_mfg_data(out_result->mfg, out_result);
}

parser_status_t extract_mfg_data(const uint8_t* adv_data, size_t adv_data_len, mfg_data_t *out)
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
			if (g_actions->on_pairing) { out_result->action = g_actions->on_pairing;}
      break;

    case PARSER_PAIRED_MSG:
      if (g_actions->on_paired) { out_result->action = g_actions->on_paired;}
      break;

    case PARSER_LOST_MSG:
			if (g_actions->on_lost) { out_result->action = g_actions->on_lost;}
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