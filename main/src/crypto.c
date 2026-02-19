#include "crypto.h"
#include "psa/crypto.h"
#include "psa/crypto_values.h"

#include <stdio.h>

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
    psa_key_id_t* private_key_id,
    psa_key_id_t* peer_key_id,
    size_t secret_key_bits,
    psa_key_id_t* secret_key_id_out
    )
{
  psa_status_t status;
  psa_key_attributes_t priv_attr = PSA_KEY_ATTRIBUTES_INIT;
  psa_key_attributes_t derived_attr = PSA_KEY_ATTRIBUTES_INIT;
  psa_key_derivation_operation_t op = PSA_KEY_DERIVATION_OPERATION_INIT;

  uint8_t peer_pub[PSA_EXPORT_PUBLIC_KEY_MAX_SIZE];
  size_t peer_pub_len = 0;

  status = psa_crypto_init();                                                                                
  if (status != PSA_SUCCESS) { return status; }

  if (secret_key_id_out == NULL) { return PSA_ERROR_INVALID_ARGUMENT; }

  /* Extract private key attributes for validation below */
  status = psa_get_key_attributes(*private_key_id, &priv_attr);

  if (status != PSA_SUCCESS) { return status; }

  psa_key_type_t type = psa_get_key_type(&priv_attr);
  size_t bits = psa_get_key_bits(&priv_attr);

  /* Validate Key Provided Is X25519 */
  if (type != PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_MONTGOMERY) ||
      bits != 255)
  {
      status = PSA_ERROR_NOT_SUPPORTED;
      goto cleanup;
  }

  /* Extract Peer's Public Key */
  status = psa_export_public_key(*peer_key_id,
                                   peer_pub,
                                   sizeof(peer_pub),
                                   &peer_pub_len);

  if (status != PSA_SUCCESS) { goto cleanup; }
  
  /* Setup HKDF */
  status = psa_key_derivation_setup(
      &op,
      PSA_ALG_KEY_AGREEMENT(PSA_ALG_ECDH, PSA_ALG_HKDF(PSA_ALG_SHA_256))
      );

  if (status != PSA_SUCCESS) { goto cleanup; }

  /* Perform Key Agreement */
  status = psa_key_derivation_key_agreement(
      &op,
      PSA_KEY_DERIVATION_INPUT_SECRET,
      *private_key_id,
      peer_pub,
      peer_pub_len);

  if (status != PSA_SUCCESS) { goto cleanup; }

  /* No salt (supply random salt in real protocols) */
  /*
  status = psa_key_derivation_input_bytes(
      &op,
      PSA_KEY_DERIVATION_INPUT_SALT,
      NULL,
      0);

  if (status != PSA_SUCCESS)
  {
    printf("Salt\n");
    goto cleanup;
  }
  */

  /* Context binding */
  const uint8_t info[] = "X25519 session key";
  status = psa_key_derivation_input_bytes(
      &op,
      PSA_KEY_DERIVATION_INPUT_INFO,
      info,
      sizeof(info) - 1);

  if (status != PSA_SUCCESS) { goto cleanup; }

  /* Configure derived AES key */
  psa_set_key_type(&derived_attr, PSA_KEY_TYPE_AES);
  psa_set_key_bits(&derived_attr, secret_key_bits);
  psa_set_key_usage_flags(&derived_attr,
                          PSA_KEY_USAGE_ENCRYPT |
                          PSA_KEY_USAGE_DECRYPT);
  psa_set_key_algorithm(&derived_attr, PSA_ALG_GCM);

  status = psa_key_derivation_output_key(
      &derived_attr,
      &op,
      secret_key_id_out);

  cleanup:
    psa_key_derivation_abort(&op);
    psa_reset_key_attributes(&priv_attr);
    psa_reset_key_attributes(&derived_attr);

  return status;
}

psa_status_t generate_shared_secret_raw_bytes(
	psa_key_id_t* private_key_id,
	uint8_t* peer_key,
	size_t peer_key_len,
	size_t secret_key_bits,
	psa_key_id_t* secret_key_id_out)
{
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_status_t status;
	uint8_t public_key_buf[PSA_EXPORT_PUBLIC_KEY_MAX_SIZE];
	 
  status = psa_crypto_init();
  if (status != PSA_SUCCESS) { printf("psa_crypto_init failed"); return status; }
   
  psa_set_key_type(&attr, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_MONTGOMERY));
  psa_set_key_bits(&attr, X25519_KEY_BITS);
	psa_set_key_usage_flags(&attr,
	   PSA_KEY_USAGE_DERIVE
	);
	psa_set_key_algorithm(&attr, PSA_ALG_ECDH);
	
	status = psa_import_key(
	    &attr,
	    public_key_buf,
	    peer_key_len,
			secret_key_id_out
	);
	if (status != PSA_SUCCESS) {
		printf("import key failed %ld",status);
		return status;
	}
	
	return PSA_SUCCESS;
}



