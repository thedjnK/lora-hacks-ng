/*
 * Copyright (c) 2025, Jamie M.
 *
 * All right reserved. This code is NOT apache or FOSS/copyleft licensed.
 */

#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>
#include "ipc_settings.h"

LOG_MODULE_DECLARE(settings, CONFIG_SETTINGS_LOG_LEVEL);

static int settings_ipc_save(struct settings_store *cs, const char *name, const char *value,
			     size_t val_len);
static int settings_ipc_save_end(struct settings_store *cs);

static const struct settings_store_itf settings_ipc_interface = {
	.csi_save = settings_ipc_save,
	.csi_save_end = settings_ipc_save_end,
};

static /*const*/ struct settings_store ipc_settings_store = {
	.cs_itf = &settings_ipc_interface,
};

static int settings_ipc_save(struct settings_store *cs, const char *name, const char *value,
			     size_t val_len)
{
	if (name == NULL || (value == NULL && val_len > 0) || strlen(name) == 0) {
		return -EINVAL;
	}

	return ipc_setting_save(name, value, val_len);
}

static int settings_ipc_save_end(struct settings_store *cs)
{
	return ipc_setting_commit();
}

int settings_backend_init(void)
{
	settings_dst_register(&ipc_settings_store);
	return 0;
}
