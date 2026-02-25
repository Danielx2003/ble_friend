#include "psa/crypto.h"
#include "unity.h"

#include "ble_callbacks.h"
#include "parser.h"
#include "ble.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


void setUp(void) {
    // set stuff up here
		psa_crypto_init();
}

void tearDown(void) {
    // clean stuff up here
}

void test_on_lost()
{
	mfg_data_t adv_packet = {
		.company_id = 0xFFFF,
		.version_mode = 0x12,
		.flags = 0x00,
	};
	
	bool res = on_lost_msg(NULL, &adv_packet);
	TEST_ASSERT_TRUE(res == true);
}

int main()
{
	UNITY_BEGIN();

	RUN_TEST(test_on_lost);
	
	return UNITY_END();
}

/*
Compile command:
gcc -o ../../../build/cbs test_callbacks.c -I../../include/ -I../../../../../Unity/src/ ../../../../../Unity/src/unity.c ../../src/ble_callbacks.c  ../../src/crypto.c ./mock/mock_disc.c  -lmbedtls -lmbedcrypto
*/