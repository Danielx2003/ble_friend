#include "crypto2.h"
#include "psa/crypto.h"
#include "psa/crypto_types.h"
#include "psa/crypto_values.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Static Variables */

static uint8_t counter = 1;

/* Static Functions */

void curve25519_clamp(uint8_t k[32])
{
    k[0]  &= 248;
    k[31] &= 127;
    k[31] |= 64;
}

static void clear_key(uint8_t *k, size_t len)
{
	memset(k, 0, len);
}

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
			return CRYPTO_ERR_UNKNOWN;
	}

}

crypto_status_t crypto_init()
{
	return psa_status_to_crypto(psa_crypto_init());
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
	crypto_key_t *secret
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
		secret->raw.data,
		32,
		&(secret->raw.len)
	);
	
	secret->type = KEY_TYPE_RAW;

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

crypto_status_t derive_ephemeral_private_key(
	crypto_key_t *secret,
	const uint8_t *info,
	size_t info_len,
	crypto_key_t *private_key)
{
	psa_status_t status;
	uint8_t ephemeral_private[32];

	psa_key_derivation_operation_t derivation = PSA_KEY_DERIVATION_OPERATION_INIT;

	status = psa_key_derivation_setup(
		&derivation,
		PSA_ALG_HKDF(PSA_ALG_SHA_256)
	);
	if (status != PSA_SUCCESS) return psa_status_to_crypto(status);;

	status = psa_key_derivation_input_bytes(
		&derivation,
		PSA_KEY_DERIVATION_INPUT_SALT,
		NULL, 
		0
	);
	if (status != PSA_SUCCESS) return psa_status_to_crypto(status);;

	status = psa_key_derivation_input_bytes(
		&derivation,
		PSA_KEY_DERIVATION_INPUT_SECRET,
		secret->raw.data,
		32
	);
	if (status != PSA_SUCCESS) return psa_status_to_crypto(status);;

	status = psa_key_derivation_input_bytes(
		&derivation,
		PSA_KEY_DERIVATION_INPUT_INFO,
		&counter, 
		sizeof(counter)
	);
	if (status != PSA_SUCCESS) return psa_status_to_crypto(status);;

	status = psa_key_derivation_output_bytes(
		&derivation,
		ephemeral_private, 
		32
	);
	if (status != PSA_SUCCESS) return psa_status_to_crypto(status);
	
	curve25519_clamp(ephemeral_private);

	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_set_key_type(
		&attr,
		PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_MONTGOMERY)
	);
	psa_set_key_bits(&attr, 255);
	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_DERIVE | PSA_KEY_USAGE_EXPORT);
	psa_set_key_algorithm(&attr, PSA_ALG_ECDH);

	status = psa_import_key(
		&attr,
		ephemeral_private, 
		32,
		&(private_key->id)
	);
	if (status != PSA_SUCCESS) return status;
	
	private_key->type = KEY_TYPE_ID;
 
	return psa_status_to_crypto(status);
}

crypto_status_t derive_public_key(
	crypto_key_t *secret,
	crypto_key_t *public_key)
{
	crypto_key_t ephemeral_priv_key;
	crypto_status_t status;
	const uint8_t info[] = "eph_private"; 
	

	status = derive_ephemeral_private_key(
	    secret,
	    info,
	    sizeof(info),
	    &ephemeral_priv_key
	);
	if (status != PSA_SUCCESS) return status;

	status = psa_export_public_key(
		ephemeral_priv_key.id,
		public_key->raw.data,
		32,
		&(public_key->raw.len)
	);
	
	public_key->type = KEY_TYPE_RAW;
	
	return status;
}

crypto_status_t derive_symmetric_aes_key_hkdf(
	crypto_key_t *secret,
	uint8_t *salt,
	uint8_t *info,
	crypto_key_t *aes_key
)
{
	psa_status_t status;

  psa_key_derivation_operation_t deriv =
      PSA_KEY_DERIVATION_OPERATION_INIT;

  status = psa_key_derivation_setup(
      &deriv,
      PSA_ALG_HKDF(PSA_ALG_SHA_256)
  );
	if (status != PSA_SUCCESS) { goto cleanup; }

  status = psa_key_derivation_input_bytes(
      &deriv,
      PSA_KEY_DERIVATION_INPUT_SALT,
      salt,
      salt == NULL ? 0 : sizeof(salt) - 1
  );
	if (status != PSA_SUCCESS) { goto cleanup; }

  status = psa_key_derivation_input_bytes(
      &deriv,
      PSA_KEY_DERIVATION_INPUT_SECRET,
			secret->raw.data,
      32
  );
	if (status != PSA_SUCCESS) { goto cleanup; }

  status = psa_key_derivation_input_bytes(
      &deriv,
      PSA_KEY_DERIVATION_INPUT_INFO,
      info,
      info == NULL ? 0 : sizeof(info) - 1
  );
	if (status != PSA_SUCCESS) { goto cleanup; }

  uint8_t finder_sym_bytes[32];

  status = psa_key_derivation_output_bytes(
      &deriv,
      finder_sym_bytes,
      32
  );
	if (status != PSA_SUCCESS) { goto cleanup; }

  /* Import finder AES key */
  psa_key_attributes_t aes_attr = PSA_KEY_ATTRIBUTES_INIT;
  psa_set_key_type(&aes_attr, PSA_KEY_TYPE_AES);
  psa_set_key_bits(&aes_attr, 256);
  psa_set_key_usage_flags(&aes_attr,
      PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);
  psa_set_key_algorithm(&aes_attr,
      PSA_ALG_GCM);

  status = psa_import_key(
      &aes_attr,
      finder_sym_bytes,
      32,
      &(aes_key->id)
  );
	if (status != PSA_SUCCESS) { goto cleanup; }

	psa_key_derivation_abort(&deriv);

	return CRYPTO_SUCCESS;

cleanup:
	psa_key_derivation_abort(&deriv);
	return psa_status_to_crypto(status);
}

crypto_status_t export_public_key(
	crypto_key_t *keypair,
	crypto_key_t *public_key
)
{
	if (keypair->type == KEY_TYPE_RAW) { return CRYPTO_ERR_INVALID_ARGS; }
	
	psa_status_t status;
	
	status = psa_export_public_key(
		keypair->id,
		public_key->raw.data,
		32,
		&(public_key->raw.len)
	);
	if (status != PSA_SUCCESS) { return psa_status_to_crypto(status); }
	
	public_key->type = KEY_TYPE_RAW;
		
	return CRYPTO_SUCCESS;
}

