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


/**
 * Generate an ECC keypair for the selected curve.
 * Returns PSA_SUCCESS on success.
 */
psa_status_t generate_keypair(
    crypto_curve_t curve,
    psa_key_id_t* keypair_out
    );

/**
 * Generate a Secret key using ECDH
 * 
 * @param private_key_id: Device's private key
 * @param peer_key_id: Peer's key
 * @param secret_key_size: ASM key size
 * @param secret_key_id_out: Out param of generated secret key
 *
 * Returns PSA_SUCCESS on success.
 */
psa_status_t generate_secret(
    psa_key_id_t* private_key_id,
    psa_key_id_t* peer_key_id,
    psa_key_id_t* secret_key_id_out
    );


psa_status_t generate_shared_secret_raw_bytes(
    psa_key_id_t* private_key_id,
    uint8_t* peer_key,
		size_t peer_key_len,
    size_t secret_key_bits,
    psa_key_id_t* secret_key_id_out
    );


psa_status_t derive_master_seed(psa_key_id_t shared_secret_id,
                                 psa_key_id_t *master_key_id);