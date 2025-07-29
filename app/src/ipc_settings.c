/*
 * Copyright (c) 2025, Jamie M.
 *
 * All right reserved. This code is NOT apache or FOSS/copyleft licensed.
 */

#include <stdlib.h>
#include <zephyr/types.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include "ipc_endpoint.h"

LOG_MODULE_REGISTER(ipc_settings, 4);

struct ipc_setting_save_data {
	uint8_t name_size;
	uint8_t value_size;
	uint8_t setting[]; //Name, followed by value
};

static int ipc_setting_callback_save(const uint8_t *message, uint16_t size, void *user_data);
static int ipc_setting_callback_load(const uint8_t *message, uint16_t size, void *user_data);

static struct ipc_group ipc_group_save = {
	.callback = ipc_setting_callback_save,
	.opcode = IPC_OPCODE_SETTINGS_SAVE,
};

static struct ipc_group ipc_group_load = {
	.callback = ipc_setting_callback_load,
	.opcode = IPC_OPCODE_SETTINGS_LOAD,
};


#if 1
//server side:
static int ipc_setting_callback_save(/*uint8_t *name, uint8_t *value, uint8_t value_size*/ const uint8_t *message, uint16_t size, void *user_data)
{
LOG_ERR("abc");
return 0;
}

static int ipc_setting_callback_load(const uint8_t *message, uint16_t size, void *user_data)
{
LOG_ERR("def");
return 0;
}
#endif

#if 1
//client side:
int ipc_setting_save(uint8_t *name, uint8_t *value, uint8_t value_size)
{
	int rc;
	struct ipc_setting_save_data *data;
	uint8_t name_size = strlen(name);
	uint8_t total_size = sizeof(struct ipc_setting_save_data) - sizeof(uint8_t *) + name_size + value_size;

	data = (struct ipc_setting_save_data *)malloc(total_size);
	data->name_size = name_size;
	data->value_size = value_size;
	memcpy(data->setting, name, name_size);
	memcpy((data->setting + name_size), value, value_size);

	rc = ipc_send_message(IPC_OPCODE_SETTINGS_SAVE, total_size, (uint8_t *)data);
	free(data);

	return rc;
}

int ipc_setting_load(uint8_t *name)
{
return -1;
}
#endif

static int ipc_settings_register(void)
{
	ipc_register(&ipc_group_save);
	ipc_register(&ipc_group_load);

	return 0;
}

SYS_INIT(ipc_settings_register, APPLICATION, 0);
