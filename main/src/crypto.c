#include "crypto.h"
#include "psa/crypto.h"
#include "psa/crypto_types.h"
#include "psa/crypto_values.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static uint8_t counter = 1;

typedef psa_key_id_t crypto_backend_key_handle_t;

crypto_status_t convert_from_id_to_raw(crypto_key_t *key);

crypto_status_t psa_status_to_crypto(psa_status_t status)
{
	switch (status)
	{
		case PSA_ERROR_INVALID_ARGUMENT:
			return CRYPTO_ERR_INVALID_ARGS;
		case PSA_SUCCESS:
			return CRYPTO_SUCCESS;
		case PSA_ERROR_INVALID_HANDLE:
			return CRYPTO_ERR_INVALID_HANDLE;
		default:
			printf("err code: %d\n", status);
			return CRYPTO_ERR_UNKNOWN;
	}

}

crypto_status_t generate_keypair(
  crypto_curve_t curve,
  crypto_key_t *keypair
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
      return psa_status_to_crypto(PSA_ERROR_INVALID_ARGUMENT);
  }

  psa_set_key_usage_flags(&attr,
     PSA_KEY_USAGE_DERIVE
  );
  psa_set_key_algorithm(&attr, PSA_ALG_ECDH);

  status = psa_generate_key(&attr, &(keypair->id));
	keypair->type = KEY_TYPE_ID;
  if (status != PSA_SUCCESS) return status;

  return psa_status_to_crypto(PSA_SUCCESS); 
}

crypto_status_t generate_secret(
	crypto_key_t *priv_key,
	crypto_key_t *peer_key,
	uint8_t *raw_secret,
	size_t *secret_len
)
{
	psa_status_t status;

	// Private Key should never be raw, as it is not sent over the air
	if (priv_key->type == KEY_TYPE_RAW)
	{
		return CRYPTO_ERR_INVALID_ARGS;
	}
	
	uint8_t peer_pub_key[PSA_EXPORT_PUBLIC_KEY_MAX_SIZE];
	size_t peer_pub_key_len = 0;	

	if (peer_key->type == KEY_TYPE_ID)
	{
		status = psa_export_public_key(
			peer_key->id,
			peer_pub_key,
			32,
		&peer_pub_key_len
		);
	}
	else
	{
		memcpy(peer_pub_key, peer_key->raw.data, peer_key->raw.len);
		peer_pub_key_len = peer_key->raw.len;
	}

	status = psa_raw_key_agreement(
		PSA_ALG_ECDH,
		priv_key->id,
		peer_pub_key,
		32,
		raw_secret,
		32,
		secret_len
	);

	return psa_status_to_crypto(status);
}

crypto_status_t convert_from_id_to_raw(crypto_key_t *key)
{
	if (key->type == KEY_TYPE_RAW) { return CRYPTO_ERR_INVALID_ARGS; }
	
	uint8_t pub_key[PSA_EXPORT_PUBLIC_KEY_MAX_SIZE];
	size_t pub_key_len = 0;	

	psa_status_t status = psa_export_public_key(
		key->id,
		pub_key,
		32,
	&pub_key_len
	);
	if (status != PSA_SUCCESS) { return psa_status_to_crypto(status); }

	key->type = KEY_TYPE_RAW;
	memcpy(key->raw.data, pub_key, pub_key_len);
	key->raw.len = pub_key_len;

	return CRYPTO_SUCCESS;
}

crypto_status_t convert_from_raw_to_id(crypto_key_t *key)
{
	psa_status_t status;

	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_set_key_type(
		&attr,
		PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_MONTGOMERY)
	);
	psa_set_key_bits(&attr, 255);
	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_DERIVE | PSA_KEY_USAGE_EXPORT);
	psa_set_key_algorithm(&attr, PSA_ALG_ECDH);

	psa_key_id_t *output_key = (psa_key_id_t *)malloc(sizeof(psa_key_id_t));

	status = psa_import_key(
		&attr,
		key->raw.data,
		key->raw.len,
		output_key
	);
	if (status != PSA_SUCCESS) { return psa_status_to_crypto(status); }

	key->type = KEY_TYPE_ID;
	key->id = *output_key;

	return CRYPTO_SUCCESS;
}

/*

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

psa_status_t generate_secret_raw_bytes(
  psa_key_id_t *priv_key,
  uint8_t peer_key[32],
	uint8_t raw_secret[32]
  )
{
  psa_status_t status;

	memset(&raw_secret[0], 0, 32);

	size_t output_len;

	status = psa_raw_key_agreement(
		PSA_ALG_ECDH,
		*priv_key,
		peer_key,
		32,
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

*/
