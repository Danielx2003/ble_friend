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
		.payload_len = 32,
		.payload = {
			    0x3d, 0x40, 0x17, 0xc3, 0xe8, 0x43, 0x89, 0x5a,
			    0x92, 0xb7, 0x0a, 0xa7, 0x4d, 0x1b, 0x7e, 0xbc,
			    0x9c, 0x98, 0x2c, 0xcf, 0x2e, 0xc4, 0x96, 0x8c,
			    0xc0, 0xcd, 0x55, 0xf1, 0x2a, 0xf4, 0x66, 0x0c
			}
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