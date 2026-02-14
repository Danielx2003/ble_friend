#include "../include/parser.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>

parser_status_t parse_adv_data(const uint8_t* adv_data, size_t adv_data_len)
{
  parser_status_t status;
  mfg_data_t mfg = {0};

  status = extract_mfg_data(adv_data, adv_data_len, &mfg);
  if (status != PARSER_SUCCESS) { return status; }
  
  return parse_mfg_data(&mfg);
}

parser_status_t extract_mfg_data(const uint8_t* adv_data, size_t adv_data_len, mfg_data_t* out)
{
  if (adv_data_len < MIN_ADV_DATA_LEN) { return PARSER_ERR_ADV_DATA_LEN; }

  int i = 0;

  while (i < adv_data_len)
  {
    uint8_t field_len = adv_data[i++];
    uint8_t type = adv_data[i++];

    if (i + field_len-2 >= adv_data_len) {
      return PARSER_ERR_MFG_DATA; 
    }

    if (type == 0xFF)
    {
      if (field_len < PROTOCOL_HEADER_SIZE) { return PARSER_ERR_PROTOCOL_HEADER; }

      out->company_id = adv_data[i] | (adv_data[i+1] << 8);
      out->version_mode = adv_data[i+2];
      out->flags = adv_data[i+3];      

      return PARSER_SUCCESS;
    }

    i += field_len-1;
  }

  return PARSER_ERR_MFG_DATA;
}

parser_status_t parse_mfg_data(mfg_data_t* mfg)
{
  if (mfg->company_id != COMPANY_ID) { return PARSER_ERR_COMPANY_ID; }

  return parse_protocol_msg((mfg->version_mode & 0x0F), mfg);
}

parser_status_t parse_protocol_msg(parser_msg_t type, mfg_data_t* mfg)
{
  switch(type)
  {
    case PARSER_PAIRING_MSG:
      return handle_pairing_msg(mfg);
    case PARSER_PAIRED_MSG:
      return handle_paired_msg(mfg);
    case PARSER_LOST_MSG:
      return handle_lost_msg(mfg);
    default:
      return PARSER_ERR_MODE;

  }

  return PARSER_SUCCESS;
}

parser_status_t handle_lost_msg(mfg_data_t* mfg)
{
  printf("\nHandle Lost Msg\n");
  return PARSER_SUCCESS;
}

parser_status_t handle_pairing_msg(mfg_data_t* mfg)
{
  return PARSER_SUCCESS;
}

parser_status_t handle_paired_msg(mfg_data_t* mfg)
{
  return PARSER_SUCCESS;
}
