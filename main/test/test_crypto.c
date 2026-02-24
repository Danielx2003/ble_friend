#include "../include/crypto.h"

#include <CUnit/Basic.h>
#include <CUnit/CUnit.h>

#include <stdio.h>
#include <psa/crypto.h>
#include "psa/crypto.h"
#include "psa/crypto_values.h"
#include <string.h>
#include <stdlib.h>

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
	crypto_key_t key;

  crypto_status_t status =  generate_keypair(
      CRYPTO_CURVE_X25519,
      &key
  );
 
  CU_ASSERT(status == CRYPTO_SUCCESS);
}

void test_generate_keypair_P256()
{ 
	crypto_key_t key;
	
  crypto_status_t status =  generate_keypair(
      CRYPTO_CURVE_P256,
      &key
  );
 
  CU_ASSERT(status == CRYPTO_SUCCESS);
}

void test_generate_secret_id_id()
{
	crypto_key_t priv_key;
	crypto_key_t pub_key;

	crypto_status_t status =  generate_keypair(
		CRYPTO_CURVE_X25519,
		&priv_key
	 );

	 CU_ASSERT(status == CRYPTO_SUCCESS);
	 
	 status =  generate_keypair(
	 	CRYPTO_CURVE_X25519,
	 	&pub_key
	  );

	  CU_ASSERT(status == CRYPTO_SUCCESS);
	
	uint8_t raw_secret[32];
	size_t raw_secret_len;
	
	status = generate_secret(
		&priv_key,
		&pub_key,
		raw_secret,
		&raw_secret_len
	);
	CU_ASSERT(status == CRYPTO_SUCCESS);
}

void test_generate_secret_id_raw()
{
	crypto_key_t priv_key;
	crypto_key_t pub_key;

	crypto_status_t status =  generate_keypair(
		CRYPTO_CURVE_X25519,
		&priv_key
	 );

	CU_ASSERT(status == CRYPTO_SUCCESS);
	 
	status =  generate_keypair(
		CRYPTO_CURVE_X25519,
		&pub_key
	);
	
	CU_ASSERT(status == CRYPTO_SUCCESS);
	
	uint8_t raw_secret[32];
	size_t raw_secret_len;
	
	status = generate_secret(
		&priv_key,
		&pub_key,
		raw_secret,
		&raw_secret_len
	);
	CU_ASSERT(status == CRYPTO_SUCCESS);
}

void test_convert_from_id_to_raw_valid_id_key()
{
	crypto_key_t key;

	crypto_status_t status =  generate_keypair(
		CRYPTO_CURVE_X25519,
		&key
	);

	CU_ASSERT(status == CRYPTO_SUCCESS);
	 
	status = convert_from_id_to_raw(&key);
	CU_ASSERT(status == CRYPTO_SUCCESS);
	CU_ASSERT(key.type == KEY_TYPE_RAW);
}

void test_convert_from_id_to_raw_invalid_id_key()
{
	crypto_key_t key = {
		.type = KEY_TYPE_ID,
		.id = 123
	};
	 
	crypto_status_t status = convert_from_id_to_raw(&key);
	CU_ASSERT(status == CRYPTO_ERR_INVALID_HANDLE);
}

void test_convert_from_id_to_raw_with_raw_key()
{
	crypto_key_t key;

	crypto_status_t status =  generate_keypair(
		CRYPTO_CURVE_X25519,
		&key
	);

	CU_ASSERT(status == CRYPTO_SUCCESS);
	 
	status = convert_from_id_to_raw(&key);
	CU_ASSERT(status == CRYPTO_SUCCESS);
	CU_ASSERT(key.type == KEY_TYPE_RAW);

	status = convert_from_id_to_raw(&key);
	CU_ASSERT(status == CRYPTO_ERR_INVALID_ARGS);
}

void test_derive_public_key_from_secret()
{
	crypto_key_t priv_key;
	crypto_key_t pub_key;

	crypto_status_t status =  generate_keypair(
		CRYPTO_CURVE_X25519,
		&priv_key
	 );

	 CU_ASSERT(status == CRYPTO_SUCCESS);
	 
	 status =  generate_keypair(
	 	CRYPTO_CURVE_X25519,
	 	&pub_key
	  );

	  CU_ASSERT(status == CRYPTO_SUCCESS);

	uint8_t raw_secret[32];
	size_t raw_secret_len;

	status = generate_secret(
		&priv_key,
		&pub_key,
		raw_secret,
		&raw_secret_len
	);
	CU_ASSERT(status == CRYPTO_SUCCESS);
	
	crypto_key_t eph_pub_key;
	
	status = derive_public_key(raw_secret, &eph_pub_key);
	CU_ASSERT(status == CRYPTO_SUCCESS);
}

void test_derive_symmetric_aes_key()
{
	crypto_key_t priv_key;
	crypto_key_t pub_key;

	crypto_status_t status =  generate_keypair(
		CRYPTO_CURVE_X25519,
		&priv_key
	 );

	 CU_ASSERT(status == CRYPTO_SUCCESS);
	 
	 status =  generate_keypair(
	 	CRYPTO_CURVE_X25519,
	 	&pub_key
	  );

	  CU_ASSERT(status == CRYPTO_SUCCESS);

	uint8_t raw_secret[32];
	size_t raw_secret_len;

	status = generate_secret(
		&priv_key,
		&pub_key,
		raw_secret,
		&raw_secret_len
	);
	CU_ASSERT(status == CRYPTO_SUCCESS);
	

	crypto_key_t secret = {
		.type = KEY_TYPE_RAW,
		.raw = {
			.data = {0},
			.len = 1
		}
	};
		
	crypto_key_t aes_key;

	derive_symmetric_aes_key_hkdf(
//		&secret,
	raw_secret,		
	NULL, NULL,
		&aes_key
	);
}

void test_enc_roundtrip()
{
	crypto_status_t status;
	crypto_key_t owner_keypair;
	crypto_key_t airtag_keypair;
	crypto_key_t finder_keypair;
	
	/* 1. Genrate Keypairs */
	
  CU_ASSERT(generate_keypair(CRYPTO_CURVE_X25519, &owner_keypair) == PSA_SUCCESS);
  CU_ASSERT(generate_keypair(CRYPTO_CURVE_X25519, &airtag_keypair) == PSA_SUCCESS);
  CU_ASSERT(generate_keypair(CRYPTO_CURVE_X25519, &finder_keypair) == PSA_SUCCESS);
	
	/* 2. Generate Master Secret */
	
	uint8_t master_secret[32] = {0};
	size_t secret_len;

  CU_ASSERT(generate_secret(
      &owner_keypair,
      &airtag_keypair,
      master_secret,
			&secret_len
  ) == PSA_SUCCESS);
	
	/* 3. Derive Ephemral rotating key*/
	
	crypto_key_t eph_pub_key;

  CU_ASSERT(derive_public_key(
      master_secret,
			&eph_pub_key
  ) == PSA_SUCCESS);
	
	/* 4. Perform ECDH with advertised key */
	
	uint8_t finder_shared_secret[32] = {0};
	size_t finder_secret_len;
	
  CU_ASSERT(generate_secret(
      &finder_keypair,
      &eph_pub_key,
      finder_shared_secret,
			&finder_secret_len
  ) == PSA_SUCCESS);
	
	/* 5. Derive Symmetric AES Key */
	
	crypto_key_t aes_key;
	
	CU_ASSERT(derive_symmetric_aes_key_hkdf(
		finder_shared_secret,		
		NULL, NULL,
			&aes_key
	) == CRYPTO_SUCCESS);
	
	/* 6. Owner re-derives eph key */
	
	crypto_key_t eph_priv;
	const uint8_t info[] = "eph_private"; 
	
	CU_ASSERT(derive_ephemeral_private_key(
		master_secret,
		info, sizeof(info),
		&eph_priv
	) == CRYPTO_SUCCESS);	
	
	/* 7. Owner perofrms ECDH with finder's public key */
	
	uint8_t owner_shared_secret[32];
	size_t owner_secret_len;
	
  status = generate_secret(
      &eph_priv,
      &finder_keypair,
      owner_shared_secret,
			&owner_secret_len
  );
	CU_ASSERT(status == CRYPTO_SUCCESS);
	
	crypto_key_t owner_aes_key;
	CU_ASSERT(derive_symmetric_aes_key_hkdf(
		owner_shared_secret,
		NULL,
		NULL,
		&owner_aes_key
	) == CRYPTO_SUCCESS);
	
	
	/* -------------------------------- */
	/*         Validation               */
	/* -------------------------------- */
	
	/* Compare Keys */
	
	uint8_t key1[32];
	size_t key1_size;
	uint8_t key2[32];
	size_t key2_size;
	
	psa_export_public_key(
		aes_key.id,
		key1,
		32,
	&key1_size
	);
	
	psa_export_public_key(
		owner_aes_key.id,
		key2,
		32,
	&key2_size
	);
	
  CU_ASSERT(memcmp(
		key1,
		key2,
		32
  ) == 0);
	
	/* Validate Encryption/Decryption */
	
  uint8_t plaintext[] = "hello airtag";
  uint8_t ciphertext[128];
  uint8_t decrypted[128];
  uint8_t nonce[12] = {0};

  size_t ciphertext_len;
  size_t decrypted_len;

  status = psa_aead_encrypt(
    aes_key.id,
    PSA_ALG_GCM,
    nonce, sizeof(nonce),
    NULL, 0,
    plaintext, sizeof(plaintext),
    ciphertext, sizeof(ciphertext),
    &ciphertext_len
  );
	
  status = psa_aead_decrypt(
    owner_aes_key.id,
    PSA_ALG_GCM,
    nonce, sizeof(nonce),
    NULL, 0,
    ciphertext, ciphertext_len,
    decrypted, sizeof(decrypted),
    &decrypted_len
  );
  CU_ASSERT(status == PSA_SUCCESS);

  CU_ASSERT(decrypted_len == sizeof(plaintext));
  CU_ASSERT(memcmp(
      plaintext,
      decrypted,
      sizeof(plaintext)
  ) == 0);
}

int main()
{
  CU_initialize_registry();
  CU_pSuite suite = CU_add_suite("Crypto Suite", 0, 0);
  setUp();

  CU_add_test(suite, "Generate Keypair Using Curve25519", test_generate_keypair_X25519);
  CU_add_test(suite, "Generate Keypair Using NIST P-256", test_generate_keypair_P256);

  CU_add_test(suite, "Generate Secret Key Id+Id", test_generate_secret_id_id);

	CU_add_test(suite, "Convert From Id To Raw Valid Id", test_convert_from_id_to_raw_valid_id_key);
	CU_add_test(suite, "Convert From Id To Raw Invalid Param", test_convert_from_id_to_raw_with_raw_key);
	CU_add_test(suite, "Convert From Id To Raw Invalid Key", test_convert_from_id_to_raw_invalid_id_key);
	
	CU_add_test(suite, "Derive Public Key From Secret", test_derive_public_key_from_secret);
	
	CU_add_test(suite, "Derive AES Key", test_derive_symmetric_aes_key);
	
	CU_add_test(suite, "Encryption Roundtrip", test_enc_roundtrip);
	
  CU_basic_run_tests();
  CU_cleanup_registry();

  return 0;
}

