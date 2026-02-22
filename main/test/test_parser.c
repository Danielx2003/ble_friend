#include "../include/parser.h"

#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

parser_status_t mock_parse_protocol_msg(parser_msg_t type, mfg_data_t* mfg, parser_result_t* out_result) {
	printf("Mock\n");
  return PARSER_SUCCESS;
}

parser_status_t (*parse_protocol_msg_ptr)(parser_msg_t, mfg_data_t*, parser_result_t*) = parse_protocol_msg;  // Default to original

void test_adv_data_parser_exists()
{
  parser_result_t result;
  uint8_t adv_data[255] = {0};
  parser_status_t status = parse_adv_data(adv_data, sizeof(adv_data), &result);
}


void test_extract_mfg_data_returns_success_when_data_ok()
{
  uint8_t adv_data[255] = {
    0x07, 0x08, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
    0x05, 0xFF, 0xFF, 0xFF, 0x13, 0x00
  };

  mfg_data_t mfg_data = {0};

  parser_status_t status = extract_mfg_data(adv_data, sizeof(adv_data), &mfg_data);

  printf("Status: %d\n", status);

  CU_ASSERT(status == PARSER_SUCCESS);
}


void test_extract_mfg_invalid_protocol_header()
{ 
  uint8_t adv_data[255] = {
    0x07, 0x08, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
    0x03, 0xFF, 0xFF, 0xFF
  };

  mfg_data_t mfg_data = {0};

  parser_status_t status = extract_mfg_data(adv_data, sizeof(adv_data), &mfg_data);
  CU_ASSERT(status == PARSER_ERR_PROTOCOL_HEADER);
}

void test_extract_mfg_data_no_data()
{ 
  uint8_t adv_data[255] = {
    0x07, 0x08, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06
  };

  mfg_data_t mfg_data = {0};

  parser_status_t status = extract_mfg_data(adv_data, sizeof(adv_data), &mfg_data);
  CU_ASSERT(status == PARSER_ERR_MFG_DATA);
}

void test_parse_mfg_data_invalid_company()
{
  parser_result_t result;
  mfg_data_t mfg_data = {
    .company_id = 0x12FF,
    .version_mode = 0x13,
    .flags = 0x00
  };
	
	parse_protocol_msg_ptr = mock_parse_protocol_msg;

  parser_status_t status = parse_mfg_data(&mfg_data, &result);

  CU_ASSERT(status == PARSER_ERR_COMPANY_ID);
}


void test_parse_mfg_data_valid_company()
{
  parser_result_t result;
  mfg_data_t mfg_data = {
    .company_id = 0xFFFF,
    .version_mode = 0x12,
    .flags = 0x00
  };
	
	parse_protocol_msg_ptr = mock_parse_protocol_msg;

	printf("Calling parse\n");
  parser_status_t status = parse_mfg_data(&mfg_data, &result);
  printf("\nStatus: %d \n", status);
  CU_ASSERT(status == PARSER_SUCCESS);
}

/*
void test_parse_protocol_valid()
{ 
  mfg_data_t mfg_data = {
    .company_id = 0xFFFF,
    .version_mode = 0x12,
    .flags = 0x00
  };

  parser_status_t status = parse_protocol_msg((mfg_data.version_mode & 0x0F), &mfg_data);

  CU_ASSERT(status == PARSER_SUCCESS);
}

void test_parse_protocol_invalid_mode()
{
  mfg_data_t mfg_data = {
    .company_id = 0xFFFF,
    .version_mode = 0x1F,
    .flags = 0x00
  };

  parser_status_t status = parse_protocol_msg((mfg_data.version_mode & 0x0F), &mfg_data);

  CU_ASSERT(status == PARSER_ERR_MODE);
}
*/
int main()
{
  CU_initialize_registry();
  CU_pSuite suite = CU_add_suite("Parsers Suite", 0, 0);

  /* Add Tests */
  // CU_add_test(suite, "Test Parser Exists", test_parser_exists);
  // CU_add_test(suite, "Test Parser Rejects Invalid CompanyId", test_parser_rejects_invalid_company_id);

  CU_add_test(suite, "Test Adv Data Parser Exists", test_adv_data_parser_exists);
  
  CU_add_test(suite, "Test Extract Mfg Data Returns Success When Data Valid", test_extract_mfg_data_returns_success_when_data_ok);
  
  CU_add_test(suite, "Test Extract Mfg Data Invalid Adv Data Len", test_extract_mfg_invalid_protocol_header);
  CU_add_test(suite, "Test Extract Mfg Data No Mfg Data", test_extract_mfg_data_no_data);

  CU_add_test(suite, "Test Parse Mfg Data Valid Company Id", test_parse_mfg_data_valid_company);
  /*
  CU_add_test(suite, "Test Parse Mfg Data Invalid Company Id", test_parse_mfg_data_invalid_company);
  
  CU_add_test(suite, "Test Parse Protocol", test_parse_protocol_valid);
  */

  CU_basic_run_tests();
  CU_cleanup_registry();
  return 0;
}

