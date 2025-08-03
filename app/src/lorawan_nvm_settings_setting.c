/*
 * Copyright (c) 2025, Jamie M.
 *
 * All right reserved. This code is NOT apache or FOSS/copyleft licensed.
 */

#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include <zephyr/logging/log.h>
#include "lorawan_nvm_settings.h"

LOG_MODULE_REGISTER(lorawan_nvm_setting, CONFIG_LORAWAN_LOG_LEVEL);

static int lora_nvm_handle_set(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg)
{
	const char *next;
	size_t name_len;
	int rc;
	const struct lorawan_nvm_setting_descr *descr = lorawan_get_nvm_settings();

	name_len = settings_name_next(name, &next);

	if (!next) {
		for (uint32_t i = 0; i < lorawan_nvm_settings_entries; i++) {
LOG_ERR("names %s vs %s", descr->name, name);

			if (strncmp(descr->name, name, name_len) == 0) {
				if (len == descr->size) {
					rc = read_cb(cb_arg, descr->data, len);

					if (rc == len) {
						rc = 0;
						*descr->loaded = true;
					}
LOG_ERR("set!");
					return rc;
				} else {
LOG_ERR("size diff %d vs %d", len, descr->size);
					return -E2BIG;
				}
			}
		}
	}

	return -ENOENT;
}

SETTINGS_STATIC_HANDLER_DEFINE(lora_nvm, LORAWAN_SETTINGS_BASE, NULL, lora_nvm_handle_set, NULL,
			       NULL);
