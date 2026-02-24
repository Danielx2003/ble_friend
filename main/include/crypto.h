#pragma once

#include "psa/crypto.h"

#include <unistd.h>
#include <stdbool.h>

#define X25519_KEY_BITS 255
#define P256_KEY_BITS   256

typedef enum {
    CRYPTO_CURVE_X25519,
    CRYPTO_CURVE_P256
} crypto_curve_t;


///**
// * Generate an ECC keypair for the selected curve.
// * Returns PSA_SUCCESS on success.
// */
//psa_status_t generate_keypair(
//  crypto_curve_t curve,
//  psa_key_id_t* keypair_out
//);
//
///**
// * Generate a Secret key using ECDH
// * 
// * @param private_key_id: Device's private key
// * @param peer_key_id: Peer's key
// * @param secret_key_size: ASM key size
// * @param secret_key_id_out: Out param of generated secret key
// *
// * Returns PSA_SUCCESS on success.
// */
//psa_status_t generate_secret(
//	psa_key_id_t* private_key_id,
//	psa_key_id_t* peer_key_id,
//	uint8_t raw_secret[32]
//);
//
//psa_status_t derive_public_key(
//	uint8_t master_secret[32],
//	uint8_t ephemeral_pub_key[PSA_EXPORT_PUBLIC_KEY_MAX_SIZE],
//	size_t *ephemeral_pub_key_size
//);
//
//psa_status_t generate_secret_raw_bytes(
//  psa_key_id_t *priv_key,
//  uint8_t peer_key[32],
//	uint8_t raw_secret[32]
//);



/* Wrapper Implementation */

typedef enum {
	CRYPTO_SUCCESS,
	CRYPTO_ERR_INVALID_ARGS
} crypto_status_t;

typedef struct crypto_key crypto_key_id_t;

typedef enum {
  KEY_TYPE_ID,
  KEY_TYPE_RAW
} key_type_t;

typedef struct {
  key_type_t type;
  union {
    crypto_key_id_t *key;
    struct {
      const uint8_t *data;
      size_t len;
    } raw;
	};
} crypto_key_t;

/**
 * Generate an ECC keypair for the selected curve.
 * Returns PSA_SUCCESS on success.
 */
crypto_status_t generate_keypair(
  crypto_curve_t curve,
	crypto_key_id_t *key
);


/*
Generate Keypair:
- takes a curve
- out param for the keypair
- returns status

Generate Secret:
- Takes two keypairs

Generate Secret with raw keys


*/