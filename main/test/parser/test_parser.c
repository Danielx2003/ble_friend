#include "unity.h"
#include "parser.h"

/* Static Variables */

bool pairing_msg_called = false;
bool paired_msg_called = false;
bool lost_msg_called = false;

/* Mock Handlers */
bool on_pairing_msg(void *ctx, mfg_data_t *mfg)
{
	pairing_msg_called = true;
	return true;
}

bool on_paired_msg(void *ctx, mfg_data_t *mfg)
{
	paired_msg_called = true;
	return true;
}

bool on_lost_msg(void *ctx, mfg_data_t *mfg)
{
	lost_msg_called = true;
	return true;
}

static parser_action_table_t ble_actions = {
  .on_pairing = on_pairing_msg,
  .on_paired = on_paired_msg,
  .on_lost = on_lost_msg
};

void setUp(void) {
    // set stuff up here
		parser_init(&ble_actions);
		pairing_msg_called = false;
		paired_msg_called = false;
		lost_msg_called = false;
}

void tearDown(void) {
    // clean stuff up here
}

void test_adv_data_parser_exists()
{
  parser_result_t result;
  const uint8_t adv_data[255] = {0};
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

  TEST_ASSERT_TRUE(status == PARSER_SUCCESS);
}

void test_extract_mfg_invalid_protocol_header()
{ 
  uint8_t adv_data[255] = {
    0x07, 0x08, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
    0x03, 0xFF, 0xFF, 0xFF
  };

  mfg_data_t mfg_data = {0};

  parser_status_t status = extract_mfg_data(adv_data, sizeof(adv_data), &mfg_data);
  TEST_ASSERT_TRUE(status == PARSER_ERR_PROTOCOL_HEADER);
}

void test_extract_mfg_data_no_data()
{ 
  uint8_t adv_data[255] = {
    0x07, 0x08, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06
  };

  mfg_data_t mfg_data = {0};

  parser_status_t status = extract_mfg_data(adv_data, sizeof(adv_data), &mfg_data);
  TEST_ASSERT_TRUE(status == PARSER_ERR_MFG_DATA);
}

void test_parse_mfg_data_invalid_company()
{
  parser_result_t result;
  mfg_data_t mfg_data = {
    .company_id = 0x12FF,
    .version_mode = 0x13,
    .flags = 0x00
  };
	
  parser_status_t status = parse_mfg_data(&mfg_data, &result);

  TEST_ASSERT_TRUE(status == PARSER_ERR_COMPANY_ID);
}

void test_parse_mfg_data_valid()
{
  parser_result_t result;
  mfg_data_t mfg_data = {
    .company_id = 0xFFFF,
    .version_mode = 0x10,
    .flags = 0x00
  };
	
  parser_status_t status = parse_mfg_data(&mfg_data, &result);
  TEST_ASSERT_TRUE(status == PARSER_SUCCESS);
	
	result.action(NULL, NULL);
	TEST_ASSERT_TRUE(pairing_msg_called == true | paired_msg_called == false | lost_msg_called == false);
}

void test_parse_protocol_pairing_msg()
{ 
  mfg_data_t mfg_data = {
    .company_id = 0xFFFF,
    .version_mode = 0x10,
    .flags = 0x00
  };
	
	parser_result_t result;
  parser_status_t status = parse_protocol_msg((mfg_data.version_mode & 0x0F), &mfg_data, &result);

  TEST_ASSERT_TRUE(status == PARSER_SUCCESS);
	
	result.action(NULL, NULL);
	TEST_ASSERT_TRUE(pairing_msg_called == true);
	TEST_ASSERT_TRUE(paired_msg_called == false);
	TEST_ASSERT_TRUE(lost_msg_called == false);
}

void test_parse_protocol_paired_msg()
{ 
  mfg_data_t mfg_data = {
    .company_id = 0xFFFF,
    .version_mode = 0x11,
    .flags = 0x00
  };
	
	parser_result_t result;
  parser_status_t status = parse_protocol_msg((mfg_data.version_mode & 0x0F), &mfg_data, &result);

  TEST_ASSERT_TRUE(status == PARSER_SUCCESS);
	
	result.action(NULL, NULL);
	TEST_ASSERT_TRUE(pairing_msg_called == false);
	TEST_ASSERT_TRUE(paired_msg_called == true);
	TEST_ASSERT_TRUE(lost_msg_called == false);
}

void test_parse_protocol_lost_msg()
{ 
  mfg_data_t mfg_data = {
    .company_id = 0xFFFF,
    .version_mode = 0x12,
    .flags = 0x00
  };
	
	parser_result_t result;
  parser_status_t status = parse_protocol_msg((mfg_data.version_mode & 0x0F), &mfg_data, &result);

  TEST_ASSERT_TRUE(status == PARSER_SUCCESS);
	
	result.action(NULL, NULL);
	TEST_ASSERT_TRUE(pairing_msg_called == false);
	TEST_ASSERT_TRUE(paired_msg_called == false);
	TEST_ASSERT_TRUE(lost_msg_called == true);
}

void test_parse_protocol_invalid_mode()
{
  mfg_data_t mfg_data = {
    .company_id = 0xFFFF,
    .version_mode = 0x1F,
    .flags = 0x00
  };
	
	parser_result_t result;
  parser_status_t status = parse_protocol_msg((mfg_data.version_mode & 0x0F), &mfg_data, &result);

  TEST_ASSERT_TRUE(status == PARSER_ERR_MODE);
	TEST_ASSERT_TRUE(pairing_msg_called == false);
	TEST_ASSERT_TRUE(paired_msg_called == false);
	TEST_ASSERT_TRUE(lost_msg_called == false);
}

int main()
{
	UNITY_BEGIN();
	
	RUN_TEST(test_adv_data_parser_exists);
	RUN_TEST(test_extract_mfg_data_returns_success_when_data_ok);
	RUN_TEST(test_extract_mfg_invalid_protocol_header);
	
	RUN_TEST(test_extract_mfg_data_no_data);
	RUN_TEST(test_parse_mfg_data_invalid_company);
	
	RUN_TEST(test_parse_mfg_data_valid);
	
	RUN_TEST(test_parse_protocol_pairing_msg);
	RUN_TEST(test_parse_protocol_paired_msg);
	RUN_TEST(test_parse_protocol_lost_msg);
		
	RUN_TEST(test_parse_protocol_invalid_mode);

	return UNITY_END();
}

