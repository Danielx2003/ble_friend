#include "crypto.h"
#include "psa/crypto.h"
#include "psa/crypto_types.h"
#include "psa/crypto_values.h"

#include <stdio.h>
#include <string.h>

static uint8_t counter = 0;

psa_status_t generate_keypair(
    crypto_curve_t curve,
    psa_key_id_t* keypair_out
    )
{
  psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
  psa_status_t status;
  
  switch (curve) {
    case CRYPTO_CURVE_X25519:
      psa_set_key_type(&attr, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_MONTGOMERY));
      psa_set_key_bits(&attr, X25519_KEY_BITS);
      break;

    case CRYPTO_CURVE_P256:
      psa_set_key_type(&attr, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
      psa_set_key_bits(&attr, P256_KEY_BITS);
      break;

    default:
      return PSA_ERROR_INVALID_ARGUMENT;
  }

  psa_set_key_usage_flags(&attr,
     PSA_KEY_USAGE_DERIVE
  );
  psa_set_key_algorithm(&attr, PSA_ALG_ECDH);

  status = psa_generate_key(&attr, keypair_out);
  if (status != PSA_SUCCESS) return status;

  return PSA_SUCCESS; 
}

psa_status_t generate_secret(
  psa_key_id_t *priv_key,
  psa_key_id_t *peer_key,
	uint8_t raw_secret[32]
  )
{
  psa_status_t status;
	
	uint8_t peer_pub_key[PSA_EXPORT_PUBLIC_KEY_MAX_SIZE];
	size_t peer_pub_key_len = 0;	
	
	status = psa_export_public_key(
		*peer_key,
		peer_pub_key,
		sizeof(peer_pub_key),
		&peer_pub_key_len
	);

		memset(&raw_secret[0], 0, 32);
	
	size_t output_len;

	status = psa_raw_key_agreement(
		PSA_ALG_ECDH,
		*priv_key,
		peer_pub_key,
		peer_pub_key_len,
		raw_secret,
		32,
		&output_len
	);

  if (status != PSA_SUCCESS) { return status; }
	
	return status;
}

psa_status_t derive_public_key(uint8_t master_secret[32], uint8_t ephemeral_pub_key[PSA_EXPORT_PUBLIC_KEY_MAX_SIZE], size_t *ephemeral_pub_key_size)
{
	psa_status_t status;
	uint8_t ephemeral_private[32];

	psa_key_derivation_operation_t derivation = PSA_KEY_DERIVATION_OPERATION_INIT;

	status = psa_key_derivation_setup(
		&derivation,
		PSA_ALG_HKDF(PSA_ALG_SHA_256)
	);
	if (status != PSA_SUCCESS) return status;
	
	status = psa_key_derivation_input_bytes(
		&derivation,
		PSA_KEY_DERIVATION_INPUT_SALT,
		NULL, 
		0
	);
	if (status != PSA_SUCCESS) return status;
	
	status = psa_key_derivation_input_bytes(
		&derivation,
		PSA_KEY_DERIVATION_INPUT_SECRET,
		master_secret, 
		32
	);
	
	if (status != PSA_SUCCESS) return status;
	
	status = psa_key_derivation_input_bytes(
		&derivation,
		PSA_KEY_DERIVATION_INPUT_INFO,
		&counter, 
		sizeof(counter)
	);
	 if (status != PSA_SUCCESS) return status;
	
	status = psa_key_derivation_output_bytes(
		&derivation,
		ephemeral_private, 
		32
	);
	 if (status != PSA_SUCCESS) return status;

	ephemeral_private[0]  &= 248;
	ephemeral_private[31] &= 127;
	ephemeral_private[31] |= 64;
	 
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_set_key_type(
		&attr,
		PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_MONTGOMERY)
	);
	psa_set_key_bits(&attr, 255);
	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_DERIVE | PSA_KEY_USAGE_EXPORT);
	psa_set_key_algorithm(&attr, PSA_ALG_ECDH);

	psa_key_id_t ephemeral_key;
	
	status = psa_import_key(
		&attr,
		ephemeral_private, 
		32,
		&ephemeral_key
	);
 	if (status != PSA_SUCCESS) return status;
	
 	memset(ephemeral_private, 0, sizeof(ephemeral_private));

 	status = psa_export_public_key(
		ephemeral_key,
		ephemeral_pub_key,
		PSA_EXPORT_PUBLIC_KEY_MAX_SIZE,
		ephemeral_pub_key_size
	);

 	psa_destroy_key(ephemeral_key);

 	return status;
}
