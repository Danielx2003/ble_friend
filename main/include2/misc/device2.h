#pragma once

#include <stdbool.h>
#include "crypto2.h"

#define NVS_ECDSA_PUB_KEY "ecdsa_pub"

bool device_init();
bool is_ecdsa_keypair_in_storage();
void write_ecdsa_public_key_to_storage(crypto_key_t *ecdsa_public_key);
void load_ecdsa_public_key_from_storage(crypto_key_t *ecdsa_public_key);
