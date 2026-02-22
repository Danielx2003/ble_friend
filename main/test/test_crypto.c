#include "../include/crypto.h"

#include <CUnit/Basic.h>
#include <CUnit/CUnit.h>

#include <stdio.h>
#include <psa/crypto.h>
#include "psa/crypto.h"
#include "psa/crypto_values.h"
#include <string.h>

void setUp(void) {
    psa_status_t status = psa_crypto_init();
		if (status == PSA_SUCCESS)
		{
			printf("Setup Valid\n");
		}
		else {
			printf("Failed to psa_crypto_init\n");
		}
}

void test_generate_keypair_X25519()
{
  psa_key_id_t key;
  
  psa_status_t res =  generate_keypair(
      CRYPTO_CURVE_X25519,
      &key
      );
  
  CU_ASSERT(res == PSA_SUCCESS);  
}

void test_generate_keypair_P256()
{ 
  psa_key_id_t key;

  psa_status_t res =  generate_keypair(
    CRYPTO_CURVE_P256,
    &key
  );
 
  CU_ASSERT(res == PSA_SUCCESS);
}

void test_generate_secret()
{
  psa_key_id_t private_key_id;
  psa_key_id_t peer_key_id;
	uint8_t raw_secret[32];
	memset(&raw_secret[0], 0, sizeof(raw_secret));


	psa_status_t status = psa_crypto_init();
	CU_ASSERT(status == PSA_SUCCESS);

  psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
  psa_set_key_type(&attr, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_MONTGOMERY));
  psa_set_key_bits(&attr, 255);
  psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_DERIVE);
  psa_set_key_algorithm(&attr, PSA_ALG_ECDH);

  psa_generate_key(&attr, &private_key_id);
  psa_generate_key(&attr, &peer_key_id);
	 
  psa_status_t res = generate_secret(
    &private_key_id,
    &peer_key_id,
		raw_secret
  );
 
  CU_ASSERT(res == PSA_SUCCESS);
}

void test_generate_secret_invalid_key()
{
	psa_key_id_t invalid = 9999;
	uint8_t raw_secret[32];

  psa_status_t res = generate_secret(&invalid, &invalid, raw_secret);

  CU_ASSERT(res != PSA_SUCCESS);
}

void test_derive_public_key()
{
  psa_key_id_t private_key_id;
  psa_key_id_t peer_key_id;
	uint8_t raw_secret[32];
	memset(&raw_secret[0], 0, sizeof(raw_secret));

  psa_status_t status = psa_crypto_init();
	CU_ASSERT(status == PSA_SUCCESS);

  psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
  psa_set_key_type(&attr, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_MONTGOMERY));
  psa_set_key_bits(&attr, 255);
  psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_DERIVE);
  psa_set_key_algorithm(&attr, PSA_ALG_ECDH);

  psa_generate_key(&attr, &private_key_id);
  psa_generate_key(&attr, &peer_key_id);
	 
  psa_status_t res = generate_secret(
    &private_key_id,
    &peer_key_id,
		raw_secret
  );
 
  CU_ASSERT(res == PSA_SUCCESS);
	
	uint8_t public_key[PSA_EXPORT_PUBLIC_KEY_MAX_SIZE];
	size_t public_key_size;
	
	status = derive_public_key(
		raw_secret, 
		public_key, 
		&public_key_size
	);

	CU_ASSERT(status == PSA_SUCCESS);
}

int main()
{
  CU_initialize_registry();
  CU_pSuite suite = CU_add_suite("Crypto Suite", 0, 0);
  setUp();

  CU_add_test(suite, "Generate Keypair Using Curve25519", test_generate_keypair_X25519);
  CU_add_test(suite, "Generate Keypair Using NIST P-256", test_generate_keypair_P256);
	
  CU_add_test(suite, "Generate Secret Key", test_generate_secret);
	CU_add_test(suite, "Generate Secret Invalid Key Fails ", test_generate_secret_invalid_key);
	
	
	CU_add_test(suite, "Derive Public Key", test_derive_public_key);
	
  CU_basic_run_tests();
  CU_cleanup_registry();
  return 0;
}

