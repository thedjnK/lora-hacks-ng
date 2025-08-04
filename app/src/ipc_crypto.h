/*
 * Copyright (c) 2025, Jamie M.
 *
 * All right reserved. This code is NOT apache or FOSS/copyleft licensed.
 */

#include <stdint.h>

enum usage_type {
	TYPE_AES128,
	TYPE_CMAC_AES128,
};

int ipc_lorawan_crypto_set_key(uint8_t *key, uint16_t key_size, uint8_t usage);
int ipc_lorawan_crypto_aes128_ecb_encrypt(uint8_t key_id, uint8_t *data, uint16_t data_size, uint8_t *encrypted_data);
int ipc_lorawan_crypto_cmac_aes128_encrypt(uint8_t key_id, uint8_t *data, uint16_t data_size, uint8_t *prior_data, uint16_t prior_data_size, uint8_t *encrypted_data);
