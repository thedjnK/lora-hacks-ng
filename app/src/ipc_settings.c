/*
 * Copyright (c) 2025, Jamie M.
 *
 * All right reserved. This code is NOT apache or FOSS/copyleft licensed.
 */

#include <stdlib.h>
#include <zephyr/types.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include "ipc_endpoint.h"

#if defined(CONFIG_IPC_SETTINGS_SERVER)
#include <zephyr/settings/settings.h>
#endif

LOG_MODULE_REGISTER(ipc_settings, 4);

struct ipc_setting_save_data {
	uint8_t name_size;
	uint8_t value_size;
	uint8_t setting[]; //Name, followed by value
};

struct ipc_setting_save_response_data {
	int rc;
};

struct ipc_setting_load_data {
	uint8_t name_size;
	uint8_t max_value_size;
	uint8_t name[];
};

struct ipc_setting_load_response_data {
	int rc;
	uint8_t value_size;
	uint8_t setting[];
};

struct ipc_setting_commit_response_data {
	int rc;
};

static int ipc_setting_callback_save(const uint8_t *message, uint16_t size, void *user_data);
static int ipc_setting_callback_load(const uint8_t *message, uint16_t size, void *user_data);
static int ipc_setting_callback_commit(const uint8_t *message, uint16_t size, void *user_data);

#if defined(CONFIG_IPC_SETTINGS_CLIENT)
static struct {
	struct k_sem busy;
	struct k_sem done;
	int rc;
	uint8_t *load_pointer;
	uint8_t load_size;
} ipc_settings_data;

static struct ipc_group ipc_group_save = {
	.callback = ipc_setting_callback_save,
	.opcode = IPC_OPCODE_SETTINGS_SAVE,
	.user_data = &ipc_settings_data,
};

static struct ipc_group ipc_group_load = {
	.callback = ipc_setting_callback_load,
	.opcode = IPC_OPCODE_SETTINGS_LOAD,
	.user_data = &ipc_settings_data,
};

static struct ipc_group ipc_group_commit = {
	.callback = ipc_setting_callback_commit,
	.opcode = IPC_OPCODE_SETTINGS_COMMIT,
	.user_data = &ipc_settings_data,
};
#endif

#if defined(CONFIG_IPC_SETTINGS_SERVER)
static struct ipc_group ipc_group_save = {
	.callback = ipc_setting_callback_save,
	.opcode = IPC_OPCODE_SETTINGS_SAVE,
};

static struct ipc_group ipc_group_load = {
	.callback = ipc_setting_callback_load,
	.opcode = IPC_OPCODE_SETTINGS_LOAD,
};

static struct ipc_group ipc_group_commit = {
	.callback = ipc_setting_callback_commit,
	.opcode = IPC_OPCODE_SETTINGS_COMMIT,
};
#endif


#if defined(CONFIG_IPC_SETTINGS_SERVER)
//server side:
static int ipc_setting_callback_save(const uint8_t *message, uint16_t size, void *user_data)
{
	int rc;
	struct ipc_setting_save_data *setting = (struct ipc_setting_save_data *)message;
	struct ipc_setting_save_response_data data;

	rc = settings_runtime_set(setting->setting, &setting->setting[setting->name_size], setting->value_size);

LOG_ERR("abc: %d for %s", rc, setting->setting);
data.rc = rc;

	rc = ipc_send_message(IPC_OPCODE_SETTINGS_SAVE, sizeof(data), (uint8_t *)&data);

	return rc;
}

static int ipc_setting_callback_load(const uint8_t *message, uint16_t size, void *user_data)
{
	int rc;
	struct ipc_setting_load_data *setting = (struct ipc_setting_load_data *)message;
	struct ipc_setting_load_response_data *data;

	data = (struct ipc_setting_load_response_data *)malloc(16);

	rc = settings_runtime_get(setting->name, data->setting, setting->max_value_size);

LOG_ERR("def: %d for %s", rc, setting->name);
data->rc = rc;

	data->value_size = (rc >= 0 ? rc : 0);

	rc = ipc_send_message(IPC_OPCODE_SETTINGS_LOAD, 16, (uint8_t *)data);
	free(data);

	return rc;
}

static int ipc_setting_callback_commit(const uint8_t *message, uint16_t size, void *user_data)
{
	int rc;
	struct ipc_setting_commit_response_data data;

	rc = settings_save();
data.rc = rc;

	rc = ipc_send_message(IPC_OPCODE_SETTINGS_COMMIT, sizeof(data), (uint8_t *)&data);

	return rc;
}
#endif

#if defined(CONFIG_IPC_SETTINGS_CLIENT)
//client side:
static int ipc_setting_callback_save(const uint8_t *message, uint16_t size, void *user_data)
{
//get error, give sem
	struct ipc_setting_save_response_data *data = (struct ipc_setting_save_response_data *)message;

	ipc_settings_data.rc = data->rc;
LOG_ERR("abc: %d", data->rc);

	k_sem_give(&ipc_settings_data.done);

	return 0;
}

static int ipc_setting_callback_load(const uint8_t *message, uint16_t size, void *user_data)
{
	struct ipc_setting_load_response_data *data = (struct ipc_setting_load_response_data *)message;
	ipc_settings_data.rc = data->rc;

//do null check on ipc_settings_data.load_pointer ? do size check on ipc_settings_data.load_size ?

LOG_HEXDUMP_ERR(data->setting, data->rc, "teh");

	memcpy(ipc_settings_data.load_pointer, data->setting, data->value_size);

	ipc_settings_data.load_pointer = NULL;
	ipc_settings_data.load_size = 0;
LOG_ERR("def: %d", data->rc);

	k_sem_give(&ipc_settings_data.done);

	return 0;
}

static int ipc_setting_callback_commit(const uint8_t *message, uint16_t size, void *user_data)
{
	struct ipc_setting_commit_response_data *data = (struct ipc_setting_commit_response_data *)message;

	ipc_settings_data.rc = data->rc;
LOG_ERR("qui: %d", data->rc);

	k_sem_give(&ipc_settings_data.done);

	return 0;
}

int ipc_setting_save(uint8_t *name, uint8_t *value, uint8_t value_size)
{
	int rc;
	struct ipc_setting_save_data *data;
	uint8_t name_size = strlen(name) + 1;
	uint8_t total_size = sizeof(struct ipc_setting_save_data) + name_size + value_size;

	rc = k_sem_take(&ipc_settings_data.busy, K_FOREVER);

	data = (struct ipc_setting_save_data *)malloc(total_size);
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

	rc = k_sem_take(&ipc_settings_data.done, K_FOREVER);

	if (rc == 0) {
		rc = ipc_settings_data.rc;
	}

finish:
	k_sem_give(&ipc_settings_data.busy);
	return rc;
}

int ipc_setting_load(uint8_t *name, uint8_t *value, uint8_t max_value_size)
{
	int rc;
	struct ipc_setting_load_data *data;
	uint8_t name_size = strlen(name) + 1;
	uint8_t total_size = sizeof(struct ipc_setting_load_data) + name_size;

	rc = k_sem_take(&ipc_settings_data.busy, K_FOREVER);

	data = (struct ipc_setting_load_data *)malloc(total_size);
	data->name_size = name_size;
	data->max_value_size = max_value_size;
	memcpy(data->name, name, name_size);
	ipc_settings_data.load_pointer = value;
	ipc_settings_data.load_size = max_value_size;

	rc = ipc_send_message(IPC_OPCODE_SETTINGS_LOAD, total_size, (uint8_t *)data);
	free(data);

//check length?
	if (rc < 0) {
		goto finish;
	}

	rc = k_sem_take(&ipc_settings_data.done, K_FOREVER);

	if (rc == 0) {
		rc = ipc_settings_data.rc;
	}

finish:
	k_sem_give(&ipc_settings_data.busy);
	return rc;
}

int ipc_setting_commit()
{
	int rc;

	rc = k_sem_take(&ipc_settings_data.busy, K_FOREVER);
	rc = ipc_send_message(IPC_OPCODE_SETTINGS_COMMIT, 0, NULL);

//check length?
	if (rc < 0) {
		goto finish;
	}

	rc = k_sem_take(&ipc_settings_data.done, K_FOREVER);

	if (rc == 0) {
		rc = ipc_settings_data.rc;
	}

finish:
	k_sem_give(&ipc_settings_data.busy);
	return rc;
}
#endif

static int ipc_settings_register(void)
{
#if defined(CONFIG_IPC_SETTINGS_CLIENT)
	k_sem_init(&ipc_settings_data.busy, 1, 1);
	k_sem_init(&ipc_settings_data.done, 0, 1);
	ipc_settings_data.load_pointer = NULL;
	ipc_settings_data.load_size = 0;
#endif

	ipc_register(&ipc_group_save);
	ipc_register(&ipc_group_load);
	ipc_register(&ipc_group_commit);

	return 0;
}

SYS_INIT(ipc_settings_register, APPLICATION, 0);
