/*
 * Copyright (c) 2025, Jamie M.
 *
 * All right reserved. This code is NOT apache or FOSS/copyleft licensed.
 */

#include <stdint.h>

int ipc_lorawan_crypto_set_key(uint8_t *key, uint16_t key_size);
int ipc_lorawan_crypto_aes128_ecb_encrypt(uint8_t key_id, uint8_t *data, uint16_t data_size, uint8_t *encrypted_data);
