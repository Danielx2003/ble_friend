#include "crypto.h"
#include "psa/crypto.h"
#include "psa/crypto_types.h"
#include "psa/crypto_values.h"

#include <stdio.h>
#include <string.h>

psa_status_t generate_keypair(
    crypto_curve_t curve,
    psa_key_id_t* keypair_out
    )
{
  psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
  psa_status_t status;
 
  status = psa_crypto_init();
  if (status != PSA_SUCCESS) { return status; }
   
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
     | PSA_KEY_USAGE_EXPORT // Export the Private Key (Public is enabled by default), remove later
  );
  psa_set_key_algorithm(&attr, PSA_ALG_ECDH);

  status = psa_generate_key(&attr, keypair_out);
  if (status != PSA_SUCCESS) return status;

  return PSA_SUCCESS; 
}

psa_status_t generate_secret(
  psa_key_id_t *priv_key,
  psa_key_id_t *peer_key,
  psa_key_id_t *secret_key
  )
{
  psa_status_t status;
  status = psa_crypto_init();
  if (status != PSA_SUCCESS) { return status; }
	
	uint8_t peer_pub_key[PSA_EXPORT_PUBLIC_KEY_MAX_SIZE];
	size_t peer_pub_key_len = 0;	
	
	status = psa_export_public_key(*peer_key,
	                                 peer_pub_key,
	                                 sizeof(peer_pub_key),
	                                 &peer_pub_key_len);
																	 
 	uint8_t raw_secret[32];
	memset(&raw_secret[0], 0, sizeof(raw_secret));
	
	size_t output_len;

	status = psa_raw_key_agreement(
			PSA_ALG_ECDH,
      *priv_key,
      peer_pub_key,
      peer_pub_key_len,
      raw_secret,
      sizeof(raw_secret),
      &output_len
      );

  if (status != PSA_SUCCESS) { return status; }
	
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_set_key_type(&attr, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_MONTGOMERY));
	psa_set_key_bits(&attr, X25519_KEY_BITS);
	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_DERIVE);
	psa_set_key_algorithm(&attr, PSA_ALG_HKDF(PSA_ALG_SHA_256));
	
	status = psa_import_key(
		&attr,
		raw_secret,
		sizeof(raw_secret),
		secret_key
	);
	
	return PSA_SUCCESS;
}
