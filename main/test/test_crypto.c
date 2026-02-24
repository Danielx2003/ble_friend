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
	
	printf("generate secret\n");
	
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

void test_generate_secret_raw_id()
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
	
	printf("generate secret\n");
	
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
	
	printf("generate secret\n");
	
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

void test_generate_secret_raw_raw()
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
	
	printf("generate secret\n");
	
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

//void test_generate_secret()
//{
//  psa_key_id_t private_key_id;
//  psa_key_id_t peer_key_id;
//	uint8_t raw_secret[32];
//	memset(&raw_secret[0], 0, sizeof(raw_secret));
//
//
//	psa_status_t status = psa_crypto_init();
//	CU_ASSERT(status == PSA_SUCCESS);
//
//  psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
//  psa_set_key_type(&attr, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_MONTGOMERY));
//  psa_set_key_bits(&attr, 255);
//  psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_DERIVE);
//  psa_set_key_algorithm(&attr, PSA_ALG_ECDH);
//
//  psa_generate_key(&attr, &private_key_id);
//  psa_generate_key(&attr, &peer_key_id);
//	 
//  psa_status_t res = generate_secret(
//    &private_key_id,
//    &peer_key_id,
//		raw_secret
//  );
// 
//  CU_ASSERT(res == PSA_SUCCESS);
//}
//
//void test_generate_secret_invalid_key()
//{
//	psa_key_id_t invalid = 9999;
//	uint8_t raw_secret[32];
//
//  psa_status_t res = generate_secret(&invalid, &invalid, raw_secret);
//
//  CU_ASSERT(res != PSA_SUCCESS);
//}

//void test_derive_public_key()
//{
//  psa_key_id_t private_key_id;
//  psa_key_id_t peer_key_id;
//	uint8_t raw_secret[32];
//	memset(&raw_secret[0], 0, sizeof(raw_secret));
//
//  psa_status_t status = psa_crypto_init();
//	CU_ASSERT(status == PSA_SUCCESS);
//
//  psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
//  psa_set_key_type(&attr, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_MONTGOMERY));
//  psa_set_key_bits(&attr, 255);
//  psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_DERIVE);
//  psa_set_key_algorithm(&attr, PSA_ALG_ECDH);
//
//  psa_generate_key(&attr, &private_key_id);
//  psa_generate_key(&attr, &peer_key_id);
//	 
//  psa_status_t res = generate_secret(
//    &private_key_id,
//    &peer_key_id,
//		raw_secret
//  );
// 
//  CU_ASSERT(res == PSA_SUCCESS);
//	
//	uint8_t public_key[PSA_EXPORT_PUBLIC_KEY_MAX_SIZE];
//	size_t public_key_size;
//	
//	status = derive_public_key(
//		raw_secret, 
//		public_key, 
//		&public_key_size
//	);
//
//	CU_ASSERT(status == PSA_SUCCESS);
//}
//
//void test_enc_roundtrip()
//{
//  psa_status_t status;
//
//  psa_crypto_init();
//
//  /* -------------------------------------------------- */
//  /* 1. Generate keypairs                              */
//  /* -------------------------------------------------- */
//
//  psa_key_id_t owner_keypair;
//  psa_key_id_t airtag_keypair;
//  psa_key_id_t finder_keypair;
//
//  CU_ASSERT(generate_keypair(CRYPTO_CURVE_X25519, &owner_keypair) == PSA_SUCCESS);
//  CU_ASSERT(generate_keypair(CRYPTO_CURVE_X25519, &airtag_keypair) == PSA_SUCCESS);
//  CU_ASSERT(generate_keypair(CRYPTO_CURVE_X25519, &finder_keypair) == PSA_SUCCESS);
//
//  /* -------------------------------------------------- */
//  /* 2. Owner + AirTag ECDH → master_secret            */
//  /* -------------------------------------------------- */
//
//  uint8_t master_secret[32] = {0};
//
//  CU_ASSERT(generate_secret(
//      &owner_keypair,
//      &airtag_keypair,
//      master_secret
//  ) == PSA_SUCCESS);
//
//  /* -------------------------------------------------- */
//  /* 3. AirTag derives rotating ephemeral key          */
//  /* -------------------------------------------------- */
//
//  uint8_t airtag_public_key[PSA_EXPORT_PUBLIC_KEY_MAX_SIZE];
//  size_t airtag_public_key_size;
//
//  CU_ASSERT(derive_public_key(
//      master_secret,
//      airtag_public_key,
//      &airtag_public_key_size
//  ) == PSA_SUCCESS);
//
//  /* -------------------------------------------------- */
//  /* 4. Finder performs ECDH with broadcast key        */
//  /* -------------------------------------------------- */
//
//  uint8_t finder_shared_secret[32] = {0};
//
//  CU_ASSERT(generate_secret_raw_bytes(
//      &finder_keypair,
//      airtag_public_key,
//      finder_shared_secret
//  ) == PSA_SUCCESS);
//
//  /* -------------------------------------------------- */
//  /* 5. Finder derives symmetric key via HKDF          */
//  /* -------------------------------------------------- */
//
//  psa_key_derivation_operation_t deriv =
//      PSA_KEY_DERIVATION_OPERATION_INIT;
//
//  status = psa_key_derivation_setup(
//      &deriv,
//      PSA_ALG_HKDF(PSA_ALG_SHA_256)
//  );
//  CU_ASSERT(status == PSA_SUCCESS);
//
//  status = psa_key_derivation_input_bytes(
//      &deriv,
//      PSA_KEY_DERIVATION_INPUT_SALT,
//      NULL,
//      0
//  );
//  CU_ASSERT(status == PSA_SUCCESS);
//
//  status = psa_key_derivation_input_bytes(
//      &deriv,
//      PSA_KEY_DERIVATION_INPUT_SECRET,
//      finder_shared_secret,
//      32
//  );
//  CU_ASSERT(status == PSA_SUCCESS);
//
//  const char info[] = "location message";
//
//  status = psa_key_derivation_input_bytes(
//      &deriv,
//      PSA_KEY_DERIVATION_INPUT_INFO,
//      info,
//      sizeof(info) - 1
//  );
//  CU_ASSERT(status == PSA_SUCCESS);
//
//  uint8_t finder_sym_bytes[32];
//
//  status = psa_key_derivation_output_bytes(
//      &deriv,
//      finder_sym_bytes,
//      32
//  );
//  CU_ASSERT(status == PSA_SUCCESS);
//
//  /* Import finder AES key */
//
//  psa_key_attributes_t aes_attr = PSA_KEY_ATTRIBUTES_INIT;
//  psa_set_key_type(&aes_attr, PSA_KEY_TYPE_AES);
//  psa_set_key_bits(&aes_attr, 256);
//  psa_set_key_usage_flags(&aes_attr,
//      PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);
//  psa_set_key_algorithm(&aes_attr,
//      PSA_ALG_GCM);
//
//  psa_key_id_t finder_aes_key;
//
//  status = psa_import_key(
//      &aes_attr,
//      finder_sym_bytes,
//      32,
//      &finder_aes_key
//  );
//  CU_ASSERT(status == PSA_SUCCESS);
//
//  /* -------------------------------------------------- */
//  /* 6. Owner reconstructs AirTag ephemeral private    */
//  /* -------------------------------------------------- */
//
//  uint8_t ephemeral_private[32];
//
//  psa_key_derivation_operation_t deriv2 =
//      PSA_KEY_DERIVATION_OPERATION_INIT;
//
//  status = psa_key_derivation_setup(
//      &deriv2,
//      PSA_ALG_HKDF(PSA_ALG_SHA_256)
//  );
//  CU_ASSERT(status == PSA_SUCCESS);
//
//  status = psa_key_derivation_input_bytes(
//      &deriv2,
//      PSA_KEY_DERIVATION_INPUT_SALT,
//      NULL,
//      0
//  );
//  CU_ASSERT(status == PSA_SUCCESS);
//
//  status = psa_key_derivation_input_bytes(
//      &deriv2,
//      PSA_KEY_DERIVATION_INPUT_SECRET,
//      master_secret,
//      32
//  );
//  CU_ASSERT(status == PSA_SUCCESS);
//
//  const uint8_t counter = 1;
//
//  status = psa_key_derivation_input_bytes(
//      &deriv2,
//      PSA_KEY_DERIVATION_INPUT_INFO,
//      &counter,
//      sizeof(counter)
//  );
//  CU_ASSERT(status == PSA_SUCCESS);
//
//  status = psa_key_derivation_output_bytes(
//      &deriv2,
//      ephemeral_private,
//      32
//  );
//  CU_ASSERT(status == PSA_SUCCESS);
//
//  /* Clamp for X25519 */
//  ephemeral_private[0]  &= 248;
//  ephemeral_private[31] &= 127;
//  ephemeral_private[31] |= 64;
//
//  psa_key_attributes_t ec_attr = PSA_KEY_ATTRIBUTES_INIT;
//  psa_set_key_type(&ec_attr,
//      PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_MONTGOMERY));
//  psa_set_key_bits(&ec_attr, 255);
//  psa_set_key_usage_flags(&ec_attr,
//      PSA_KEY_USAGE_DERIVE | PSA_KEY_USAGE_EXPORT);
//  psa_set_key_algorithm(&ec_attr, PSA_ALG_ECDH);
//
//  psa_key_id_t ephemeral_key;
//
//  status = psa_import_key(
//      &ec_attr,
//      ephemeral_private,
//      32,
//      &ephemeral_key
//  );
//  CU_ASSERT(status == PSA_SUCCESS);
//
//  /* -------------------------------------------------- */
//  /* 7. Owner performs ECDH with finder public key     */
//  /* -------------------------------------------------- */
//
//  uint8_t owner_shared_secret[32] = {0};
//
//  CU_ASSERT(generate_secret(
//      &ephemeral_key,
//      &finder_keypair,
//      owner_shared_secret
//  ) == PSA_SUCCESS);
//
//  /* -------------------------------------------------- */
//  /* 8. Owner derives symmetric key                    */
//  /* -------------------------------------------------- */
//
//  psa_key_derivation_operation_t deriv3 =
//      PSA_KEY_DERIVATION_OPERATION_INIT;
//
//  status = psa_key_derivation_setup(
//      &deriv3,
//      PSA_ALG_HKDF(PSA_ALG_SHA_256)
//  );
//  CU_ASSERT(status == PSA_SUCCESS);
//
//  status = psa_key_derivation_input_bytes(
//      &deriv3,
//      PSA_KEY_DERIVATION_INPUT_SALT,
//      NULL,
//      0
//  );
//  CU_ASSERT(status == PSA_SUCCESS);
//
//  status = psa_key_derivation_input_bytes(
//      &deriv3,
//      PSA_KEY_DERIVATION_INPUT_SECRET,
//      owner_shared_secret,
//      32
//  );
//  CU_ASSERT(status == PSA_SUCCESS);
//
//  status = psa_key_derivation_input_bytes(
//      &deriv3,
//      PSA_KEY_DERIVATION_INPUT_INFO,
//      info,
//      sizeof(info) - 1
//  );
//  CU_ASSERT(status == PSA_SUCCESS);
//
//  uint8_t owner_sym_bytes[32];
//
//  status = psa_key_derivation_output_bytes(
//      &deriv3,
//      owner_sym_bytes,
//      32
//  );
//  CU_ASSERT(status == PSA_SUCCESS);
//
//  /* -------------------------------------------------- */
//  /* 9. Keys must match                                */
//  /* -------------------------------------------------- */
//
//  CU_ASSERT(memcmp(
//      finder_sym_bytes,
//      owner_sym_bytes,
//      32
//  ) == 0);
//
//  /* -------------------------------------------------- */
//  /* 10. Encrypt + Decrypt test                        */
//  /* -------------------------------------------------- */
//
//  uint8_t plaintext[] = "hello airtag";
//  uint8_t ciphertext[128];
//  uint8_t decrypted[128];
//  uint8_t nonce[12] = {0};
//
//  size_t ciphertext_len;
//  size_t decrypted_len;
//
//  status = psa_aead_encrypt(
//      finder_aes_key,
//      PSA_ALG_GCM,
//      nonce, sizeof(nonce),
//      NULL, 0,
//      plaintext, sizeof(plaintext),
//      ciphertext, sizeof(ciphertext),
//      &ciphertext_len
//  );
//  CU_ASSERT(status == PSA_SUCCESS);
//
//  psa_key_id_t owner_aes_key;
//
//  status = psa_import_key(
//      &aes_attr,
//      owner_sym_bytes,
//      32,
//      &owner_aes_key
//  );
//  CU_ASSERT(status == PSA_SUCCESS);
//
//  status = psa_aead_decrypt(
//      owner_aes_key,
//      PSA_ALG_GCM,
//      nonce, sizeof(nonce),
//      NULL, 0,
//      ciphertext, ciphertext_len,
//      decrypted, sizeof(decrypted),
//      &decrypted_len
//  );
//  CU_ASSERT(status == PSA_SUCCESS);
//
//  CU_ASSERT(decrypted_len == sizeof(plaintext));
//  CU_ASSERT(memcmp(
//      plaintext,
//      decrypted,
//      sizeof(plaintext)
//  ) == 0);
//
//  /* Cleanup */
//
//  psa_destroy_key(owner_keypair);
//  psa_destroy_key(airtag_keypair);
//  psa_destroy_key(finder_keypair);
//  psa_destroy_key(ephemeral_key);
//  psa_destroy_key(finder_aes_key);
//  psa_destroy_key(owner_aes_key);
//}

int main()
{
  CU_initialize_registry();
  CU_pSuite suite = CU_add_suite("Crypto Suite", 0, 0);
  setUp();

  CU_add_test(suite, "Generate Keypair Using Curve25519", test_generate_keypair_X25519);
  CU_add_test(suite, "Generate Keypair Using NIST P-256", test_generate_keypair_P256);

  CU_add_test(suite, "Generate Secret Key", test_generate_secret_id_id);
//	CU_add_test(suite, "Generate Secret Invalid Key Fails ", test_generate_secret_invalid_key);
	
//	CU_add_test(suite, "Derive Public Key", test_derive_public_key);
//	
//	CU_add_test(suite, "Encryption Roundtrip", test_enc_roundtrip);

  CU_basic_run_tests();
  CU_cleanup_registry();

  return 0;
}

