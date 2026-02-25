#include "unity.h"
#include "crypto.h"

#include "psa/crypto.h"

#include <string.h>

void setUp(void) {
    // set stuff up here
		psa_crypto_init();
}

void tearDown(void) {
    // clean stuff up here
}

/* ----------------------- */
/* Generate Keypairs Tests */
/* ----------------------- */

void test_generate_keypair_X25519()
{
	crypto_key_t key;

	crypto_status_t status = generate_keypair(
		CRYPTO_CURVE_X25519,
		&key
	);
 
  TEST_ASSERT_TRUE(status == CRYPTO_SUCCESS);
}

void test_generate_keypair_P256()
{ 
	crypto_key_t key;

  crypto_status_t status = generate_keypair(
		CRYPTO_CURVE_P256,
		&key
  );

  TEST_ASSERT_TRUE(status == CRYPTO_SUCCESS);
}

/* --------------------- */
/* Generate Secret Tests */
/* --------------------- */

void test_generate_secret_id_id()
{
	crypto_key_t priv_key;
	crypto_key_t pub_key;

	crypto_status_t status = generate_keypair(
		CRYPTO_CURVE_X25519,
		&priv_key
	 );

	TEST_ASSERT_TRUE(status == CRYPTO_SUCCESS);
	 
	status = generate_keypair(
	 	CRYPTO_CURVE_X25519,
	 	&pub_key
	);

	TEST_ASSERT_TRUE(status == CRYPTO_SUCCESS);
		
	crypto_key_t secret;
	
	status = generate_secret(
		&priv_key,
		&pub_key,
		&secret
	);
	TEST_ASSERT_TRUE(status == CRYPTO_SUCCESS);
}

void test_generate_secret_id_raw()
{
	crypto_key_t priv_key;
	crypto_key_t pub_key;

	crypto_status_t status =  generate_keypair(
		CRYPTO_CURVE_X25519,
		&priv_key
	 );

	TEST_ASSERT_TRUE(status == CRYPTO_SUCCESS);
	 
	status =  generate_keypair(
		CRYPTO_CURVE_X25519,
		&pub_key
	);
	
	TEST_ASSERT_TRUE(status == CRYPTO_SUCCESS);
	
	crypto_key_t secret;
	
	status = generate_secret(
		&priv_key,
		&pub_key,
		&secret
	);
	TEST_ASSERT_TRUE(status == CRYPTO_SUCCESS);
}

/* Convert From/To Tests */

void test_convert_from_id_to_raw_valid_id_key()
{
	crypto_key_t key;

	crypto_status_t status =  generate_keypair(
		CRYPTO_CURVE_X25519,
		&key
	);

	TEST_ASSERT_TRUE(status == CRYPTO_SUCCESS);
	 
	status = convert_from_id_to_raw(&key);
	TEST_ASSERT_TRUE(status == CRYPTO_SUCCESS);
	TEST_ASSERT_TRUE(key.type == KEY_TYPE_RAW);
}

void test_convert_from_id_to_raw_invalid_id_key()
{
	crypto_key_t key = {
		.type = KEY_TYPE_ID,
		.id = 123
	};
	 
	crypto_status_t status = convert_from_id_to_raw(&key);
	TEST_ASSERT_TRUE(status == CRYPTO_ERR_INVALID_HANDLE);
}

void test_convert_from_id_to_raw_with_raw_key()
{
	crypto_key_t key;

	crypto_status_t status =  generate_keypair(
		CRYPTO_CURVE_X25519,
		&key
	);

	TEST_ASSERT_TRUE(status == CRYPTO_SUCCESS);
	 
	status = convert_from_id_to_raw(&key);
	TEST_ASSERT_TRUE(status == CRYPTO_SUCCESS);
	TEST_ASSERT_TRUE(key.type == KEY_TYPE_RAW);

	status = convert_from_id_to_raw(&key);
	TEST_ASSERT_TRUE(status == CRYPTO_ERR_INVALID_ARGS);
}

/* Derive Public Key Tests */

void test_derive_public_key_from_secret()
{
	crypto_key_t priv_key;
	crypto_key_t pub_key;

	crypto_status_t status =  generate_keypair(
		CRYPTO_CURVE_X25519,
		&priv_key
	 );

	 TEST_ASSERT_TRUE(status == CRYPTO_SUCCESS);
	 
	 status =  generate_keypair(
	 	CRYPTO_CURVE_X25519,
	 	&pub_key
	  );

	  TEST_ASSERT_TRUE(status == CRYPTO_SUCCESS);

	crypto_key_t secret;
		
	status = generate_secret(
		&priv_key,
		&pub_key,
		&secret
	);
	TEST_ASSERT_TRUE(status == CRYPTO_SUCCESS);
	
	crypto_key_t eph_pub_key;
	
	status = derive_public_key(&secret, &eph_pub_key);
	TEST_ASSERT_TRUE(status == CRYPTO_SUCCESS);
}


/* Derive AES Key Tests */

void test_derive_symmetric_aes_key()
{
	crypto_key_t priv_key;
	crypto_key_t pub_key;

	crypto_status_t status =  generate_keypair(
		CRYPTO_CURVE_X25519,
		&priv_key
	 );

	 TEST_ASSERT_TRUE(status == CRYPTO_SUCCESS);
	 
	 status =  generate_keypair(
	 	CRYPTO_CURVE_X25519,
	 	&pub_key
	  );

	  TEST_ASSERT_TRUE(status == CRYPTO_SUCCESS);

	crypto_key_t secret;

	status = generate_secret(
		&priv_key,
		&pub_key,
		&secret
	);
	TEST_ASSERT_TRUE(status == CRYPTO_SUCCESS);
		
	crypto_key_t aes_key;

	status = derive_symmetric_aes_key_hkdf(
	&secret,		
	NULL, NULL,
		&aes_key
	);
	TEST_ASSERT_TRUE(status == CRYPTO_SUCCESS);
}

void test_enc_roundtrip()
{
	crypto_status_t status;
	crypto_key_t owner_keypair;
	crypto_key_t airtag_keypair;
	crypto_key_t finder_keypair;
	
	/* 1. Genrate Keypairs */
	
  TEST_ASSERT_TRUE(generate_keypair(CRYPTO_CURVE_X25519, &owner_keypair) == PSA_SUCCESS);
  TEST_ASSERT_TRUE(generate_keypair(CRYPTO_CURVE_X25519, &airtag_keypair) == PSA_SUCCESS);
  TEST_ASSERT_TRUE(generate_keypair(CRYPTO_CURVE_X25519, &finder_keypair) == PSA_SUCCESS);
	
	/* 2. Generate Master Secret */
	crypto_key_t master_secret;

  TEST_ASSERT_TRUE(generate_secret(
      &owner_keypair,
      &airtag_keypair,
			&master_secret
  ) == PSA_SUCCESS);
	
	/* 3. Derive Ephemral rotating key*/
	
	crypto_key_t eph_pub_key;

  TEST_ASSERT_TRUE(derive_public_key(
      &master_secret,
			&eph_pub_key
  ) == PSA_SUCCESS);
	
	/* 4. Perform ECDH with advertised key */
	
	crypto_key_t finder_shared_secret;
	
  TEST_ASSERT_TRUE(generate_secret(
      &finder_keypair,
      &eph_pub_key,
			&finder_shared_secret
  ) == PSA_SUCCESS);
	
	/* 5. Derive Symmetric AES Key */
	
	crypto_key_t aes_key;
	
	TEST_ASSERT_TRUE(derive_symmetric_aes_key_hkdf(
		&finder_shared_secret,		
		NULL, NULL,
			&aes_key
	) == CRYPTO_SUCCESS);
	
	/* 6. Owner re-derives eph key */
	
	crypto_key_t eph_priv;
	const uint8_t info[] = "eph_private"; 
	
	TEST_ASSERT_TRUE(derive_ephemeral_private_key(
		&master_secret,
		info, sizeof(info),
		&eph_priv
	) == CRYPTO_SUCCESS);	
	
	/* 7. Owner perofrms ECDH with finder's public key */
	
	crypto_key_t owner_shared_secret;
	
  status = generate_secret(
      &eph_priv,
      &finder_keypair,
      &owner_shared_secret
  );
	TEST_ASSERT_TRUE(status == CRYPTO_SUCCESS);
	
	crypto_key_t owner_aes_key;
	TEST_ASSERT_TRUE(derive_symmetric_aes_key_hkdf(
		&owner_shared_secret,
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
	
  TEST_ASSERT_TRUE(memcmp(
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
  TEST_ASSERT_TRUE(status == PSA_SUCCESS);

  TEST_ASSERT_TRUE(decrypted_len == sizeof(plaintext));
  TEST_ASSERT_TRUE(memcmp(
      plaintext,
      decrypted,
      sizeof(plaintext)
  ) == 0);
}

int main()
{
	UNITY_BEGIN();

	/* Generate Keypair Tests */
	RUN_TEST(test_generate_keypair_X25519);
	RUN_TEST(test_generate_keypair_P256);
	
	/* Generate Secret Tests */
	RUN_TEST(test_generate_secret_id_id);
	RUN_TEST(test_generate_secret_id_raw);

	/* Convert From/To Tests */
	RUN_TEST(test_convert_from_id_to_raw_valid_id_key);
	RUN_TEST(test_convert_from_id_to_raw_invalid_id_key);
	RUN_TEST(test_convert_from_id_to_raw_with_raw_key);
	
	/* Derive Public Key Tests */
	RUN_TEST(test_derive_public_key_from_secret);
	
	/* Derive AES Key Tests */
	RUN_TEST(test_derive_symmetric_aes_key);
	RUN_TEST(test_enc_roundtrip);

	return UNITY_END();
}

/*
gcc -o ../../../build/crypto test_crypto.c -I../../include/ -I../../../../../Unity/src/ ../../../../../Unity/src/unity.c ../../src/crypto.c -lmbedtls -lmbedcrypto
*/

