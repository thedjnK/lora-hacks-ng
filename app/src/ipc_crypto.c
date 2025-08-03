/*
 * Copyright (c) 2025, Jamie M.
 *
 * All right reserved. This code is NOT apache or FOSS/copyleft licensed.
 */

#include <zephyr/types.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include "ipc_endpoint.h"

#if defined(CONFIG_IPC_LORAWAN_CRYPTO_SERVER)
#elif defined(CONFIG_IPC_LORAWAN_CRYPTO_CLIENT)
#endif

LOG_MODULE_REGISTER(ipc_crypto, 4);

struct ipc_lorawan_crypto_set_key_data {
	uint16_t key_size;
	uint8_t key[];
};

struct ipc_lorawan_crypto_set_key_response_data {
	int rc;
};

struct ipc_lorawan_crypto_aes128_encrypt_data {
	uint8_t key_id;
	uint16_t data_size;
	uint8_t data[];
};

struct ipc_lorawan_crypto_aes128_encrypt_response_data {
	int rc;
	uint16_t data_size;
	uint8_t data[];
};

struct ipc_lorawan_crypto_cmac_aes128_verify_data {
	uint8_t key_id;
	uint16_t data_size;
	uint16_t signature_size;
	uint8_t data[]; //Data followed by signature
};

struct ipc_lorawan_crypto_cmac_aes128_verify_response_data {
	int rc;
};

/* Client -> server */
static int ipc_lorawan_crypto_callback_set_key(const uint8_t *message, uint16_t size, void *user_data);
static int ipc_lorawan_crypto_callback_aes128_ecb_encrypt(const uint8_t *message, uint16_t size, void *user_data);
static int ipc_lorawan_crypto_callback_aes128_ccm_encrypt(const uint8_t *message, uint16_t size, void *user_data);
static int ipc_lorawan_crypto_callback_cmac_aes128_encrypt(const uint8_t *message, uint16_t size, void *user_data);
static int ipc_lorawan_crypto_callback_cmac_aes128_verify(const uint8_t *message, uint16_t size, void *user_data);

#if defined(CONFIG_IPC_LORAWAN_CRYPTO_CLIENT)
static struct {
	struct k_sem busy;
	struct k_sem done;
	int rc;
	uint8_t *load_pointer;
	uint8_t load_size;
} ipc_lorawan_crypto_data;

static struct ipc_group ipc_group_set_key = {
	.callback = ipc_lorawan_crypto_callback_set_key,
	.opcode = IPC_OPCODE_CRYPTO_SET_KEY,
	.user_data = &ipc_lorawan_crypto_data,
};

static struct ipc_group ipc_group_aes128_ecb_encrypt = {
	.callback = ipc_lorawan_crypto_callback_aes128_ecb_encrypt,
	.opcode = IPC_OPCODE_CRYPTO_AES128_ECB_ENCRYPT,
	.user_data = &ipc_lorawan_crypto_data,
};

static struct ipc_group ipc_group_aes128_ccm_encrypt = {
	.callback = ipc_lorawan_crypto_callback_aes128_ccm_encrypt,
	.opcode = IPC_OPCODE_CRYPTO_AES128_CCM_ENCRYPT,
	.user_data = &ipc_lorawan_crypto_data,
};

static struct ipc_group ipc_group_cmac_aes128_encrypt = {
	.callback = ipc_lorawan_crypto_callback_cmac_aes128_encrypt,
	.opcode = IPC_OPCODE_CRYPTO_CMAC_AES128_ENCRYPT,
	.user_data = &ipc_lorawan_crypto_data,
};

static struct ipc_group ipc_group_cmac_aes128_verify = {
	.callback = ipc_lorawan_crypto_callback_cmac_aes128_verify,
	.opcode = IPC_OPCODE_CRYPTO_CMAC_AES128_VERIFY,
	.user_data = &ipc_lorawan_crypto_data,
};
#endif

#if defined(CONFIG_IPC_LORAWAN_CRYPTO_SERVER)
static struct ipc_group ipc_group_set_key = {
	.callback = ipc_lorawan_crypto_callback_set_key,
	.opcode = IPC_OPCODE_CRYPTO_SET_KEY,
};

static struct ipc_group ipc_group_aes128_ecb_encrypt = {
	.callback = ipc_lorawan_crypto_callback_aes128_ecb_encrypt,
	.opcode = IPC_OPCODE_CRYPTO_AES128_ECB_ENCRYPT,
};

static struct ipc_group ipc_group_aes128_ccm_encrypt = {
	.callback = ipc_lorawan_crypto_callback_aes128_ccm_encrypt,
	.opcode = IPC_OPCODE_CRYPTO_AES128_CCM_ENCRYPT,
};

static struct ipc_group ipc_group_cmac_aes128_encrypt = {
	.callback = ipc_lorawan_crypto_callback_cmac_aes128_encrypt,
	.opcode = IPC_OPCODE_CRYPTO_CMAC_AES128_ENCRYPT,
};

static struct ipc_group ipc_group_cmac_aes128_verify = {
	.callback = ipc_lorawan_crypto_callback_cmac_aes128_verify,
	.opcode = IPC_OPCODE_CRYPTO_CMAC_AES128_VERIFY,
};
#endif

#if defined(CONFIG_IPC_LORAWAN_CRYPTO_SERVER)
static int ipc_lorawan_crypto_callback_set_key(const uint8_t *message, uint16_t size, void *user_data)
{
	int rc;
	struct ipc_lorawan_crypto_set_key_data *setting = (struct ipc_lorawan_crypto_set_key_data *)message;
	struct ipc_lorawan_crypto_set_key_response_data data;

LOG_ERR("abc: %d", rc);
data.rc = rc;

	rc = ipc_send_message(IPC_OPCODE_CRYPTO_SET_KEY, sizeof(data), (uint8_t *)&data);

	return rc;
}

static int ipc_lorawan_crypto_callback_aes128_ecb_encrypt(const uint8_t *message, uint16_t size, void *user_data)
{
	int rc;
	struct ipc_lorawan_crypto_aes128_encrypt_data *setting = (struct ipc_lorawan_crypto_aes128_encrypt_data *)message;
	struct ipc_lorawan_crypto_aes128_encrypt_response_data data;

LOG_ERR("abc: %d", rc);
data.rc = rc;

	rc = ipc_send_message(IPC_OPCODE_CRYPTO_AES128_ECB_ENCRYPT, sizeof(data), (uint8_t *)&data);

	return rc;
}

static int ipc_lorawan_crypto_callback_aes128_ccm_encrypt(const uint8_t *message, uint16_t size, void *user_data)
{
	int rc;
	struct ipc_lorawan_crypto_aes128_encrypt_data *setting = (struct ipc_lorawan_crypto_aes128_encrypt_data *)message;
	struct ipc_lorawan_crypto_aes128_encrypt_response_data data;

LOG_ERR("abc: %d", rc);
data.rc = rc;

	rc = ipc_send_message(IPC_OPCODE_CRYPTO_AES128_CCM_ENCRYPT, sizeof(data), (uint8_t *)&data);

	return rc;
}

static int ipc_lorawan_crypto_callback_cmac_aes128_encrypt(const uint8_t *message, uint16_t size, void *user_data)
{
	int rc;
	struct ipc_lorawan_crypto_aes128_encrypt_data *setting = (struct ipc_lorawan_crypto_aes128_encrypt_data *)message;
	struct ipc_lorawan_crypto_aes128_encrypt_response_data data;

LOG_ERR("abc: %d", rc);
data.rc = rc;

	rc = ipc_send_message(IPC_OPCODE_CRYPTO_CMAC_AES128_ENCRYPT, sizeof(data), (uint8_t *)&data);

	return rc;
}

static int ipc_lorawan_crypto_callback_cmac_aes128_verify(const uint8_t *message, uint16_t size, void *user_data)
{
	int rc;
	struct ipc_lorawan_crypto_cmac_aes128_verify_data *setting = (struct ipc_lorawan_crypto_cmac_aes128_verify_data *)message;
	struct ipc_lorawan_crypto_cmac_aes128_verify_response_data data;

LOG_ERR("abc: %d", rc);
data.rc = rc;

	rc = ipc_send_message(IPC_OPCODE_CRYPTO_CMAC_AES128_VERIFY, sizeof(data), (uint8_t *)&data);

	return rc;
}
#endif

#if defined(CONFIG_IPC_LORAWAN_CRYPTO_CLIENT)
static int ipc_lorawan_crypto_callback_set_key(const uint8_t *message, uint16_t size, void *user_data)
{
//get error, give sem
	struct ipc_lorawan_crypto_set_key_response_data *data = (struct ipc_lorawan_crypto_set_key_response_data *)message;

	ipc_lorawan_crypto_data.rc = data->rc;
LOG_ERR("abc: %d", data->rc);

	k_sem_give(&ipc_lorawan_crypto_data.done);

	return 0;
}

static int ipc_lorawan_crypto_callback_aes128_ecb_encrypt(const uint8_t *message, uint16_t size, void *user_data)
{
//get error, give sem
	struct ipc_lorawan_crypto_aes128_encrypt_response_data *data = (struct ipc_lorawan_crypto_aes128_encrypt_response_data *)message;

	ipc_lorawan_crypto_data.rc = data->rc;
LOG_ERR("abc: %d", data->rc);

	k_sem_give(&ipc_lorawan_crypto_data.done);

	return 0;
}

static int ipc_lorawan_crypto_callback_aes128_ccm_encrypt(const uint8_t *message, uint16_t size, void *user_data)
{
//get error, give sem
	struct ipc_lorawan_crypto_aes128_encrypt_response_data *data = (struct ipc_lorawan_crypto_aes128_encrypt_response_data *)message;

	ipc_lorawan_crypto_data.rc = data->rc;
//LOG_ERR("abc: %d", data->rc);

	k_sem_give(&ipc_lorawan_crypto_data.done);

	return 0;
}

static int ipc_lorawan_crypto_callback_cmac_aes128_encrypt(const uint8_t *message, uint16_t size, void *user_data)
{
//get error, give sem
	struct ipc_lorawan_crypto_aes128_encrypt_response_data *data = (struct ipc_lorawan_crypto_aes128_encrypt_response_data *)message;

	ipc_lorawan_crypto_data.rc = data->rc;
LOG_ERR("abc: %d", data->rc);

	k_sem_give(&ipc_lorawan_crypto_data.done);

	return 0;
}

static int ipc_lorawan_crypto_callback_cmac_aes128_verify(const uint8_t *message, uint16_t size, void *user_data)
{
//get error, give sem
	struct ipc_lorawan_crypto_cmac_aes128_verify_response_data *data = (struct ipc_lorawan_crypto_cmac_aes128_verify_response_data *)message;

	ipc_lorawan_crypto_data.rc = data->rc;
LOG_ERR("abc: %d", data->rc);

	k_sem_give(&ipc_lorawan_crypto_data.done);

	return 0;
}

#if 0
int ipc_lorawan_crypto_save(uint8_t *name, uint8_t *value, uint8_t value_size)
{
	int rc;
	struct ipc_lorawan_crypto_save_data *data;
	uint8_t name_size = strlen(name) + 1;
	uint16_t total_size = sizeof(struct ipc_lorawan_crypto_save_data) + name_size + value_size;

	rc = k_sem_take(&ipc_lorawan_crypto_data.busy, K_FOREVER);

	data = (struct ipc_lorawan_crypto_save_data *)malloc(total_size);
	data->name_size = name_size;
	data->value_size = value_size;
	memcpy(data->setting, name, name_size);
	memcpy((data->setting + name_size), value, value_size);

	rc = ipc_send_message(IPC_OPCODE_SETTINGS_SAVE, total_size, (uint8_t *)data);
	free(data);

//check length?
	if (rc < 0) {
		goto finish;
	}

	rc = k_sem_take(&ipc_lorawan_crypto_data.done, K_FOREVER);

	if (rc == 0) {
		rc = ipc_lorawan_crypto_data.rc;
	}

finish:
	k_sem_give(&ipc_lorawan_crypto_data.busy);
	return rc;
}

int ipc_lorawan_crypto_load(uint8_t *name, uint8_t *value, uint8_t max_value_size)
{
	int rc;
	struct ipc_lorawan_crypto_load_data *data;
	uint8_t name_size = strlen(name) + 1;
	uint16_t total_size = sizeof(struct ipc_lorawan_crypto_load_data) + name_size;

	rc = k_sem_take(&ipc_lorawan_crypto_data.busy, K_FOREVER);

	data = (struct ipc_lorawan_crypto_load_data *)malloc(total_size);
	data->name_size = name_size;
	data->max_value_size = max_value_size;
	memcpy(data->name, name, name_size);
	ipc_lorawan_crypto_data.load_pointer = value;
	ipc_lorawan_crypto_data.load_size = max_value_size;

	rc = ipc_send_message(IPC_OPCODE_SETTINGS_LOAD, total_size, (uint8_t *)data);
	free(data);

//check length?
	if (rc < 0) {
		goto finish;
	}

	rc = k_sem_take(&ipc_lorawan_crypto_data.done, K_FOREVER);

	if (rc == 0) {
		rc = ipc_lorawan_crypto_data.rc;
	}

finish:
	k_sem_give(&ipc_lorawan_crypto_data.busy);
	return rc;
}
#endif
#endif

static int ipc_lorawan_crypto_register(void)
{
#if defined(CONFIG_IPC_LORAWAN_CRYPTO_CLIENT)
	k_sem_init(&ipc_lorawan_crypto_data.busy, 1, 1);
	k_sem_init(&ipc_lorawan_crypto_data.done, 0, 1);
	ipc_lorawan_crypto_data.load_pointer = NULL;
	ipc_lorawan_crypto_data.load_size = 0;
#endif

	ipc_register(&ipc_group_set_key);
	ipc_register(&ipc_group_aes128_ecb_encrypt);
	ipc_register(&ipc_group_aes128_ccm_encrypt);
	ipc_register(&ipc_group_cmac_aes128_encrypt);
	ipc_register(&ipc_group_cmac_aes128_verify);

	return 0;
}

SYS_INIT(ipc_lorawan_crypto_register, APPLICATION, 0);
