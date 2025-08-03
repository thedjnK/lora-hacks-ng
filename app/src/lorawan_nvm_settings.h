/*
 * Copyright (c) 2022 Intellinium <giuliano.franchetto@intellinium.com>
 * Copyright (c) 2025, Jamie M.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef APP_LORAWAN_NVM_SETTINGS_H
#define APP_LORAWAN_NVM_SETTINGS_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#define LORAWAN_SETTINGS_BASE "lorawan/nvm"

struct lorawan_nvm_setting_descr {
	const char *name;
	const char *setting_name;
	uint8_t *data;
	size_t size;
	off_t offset;
	uint16_t flag;
	bool *loaded;
};

extern const uint16_t lorawan_nvm_settings_entries;

const struct lorawan_nvm_setting_descr *lorawan_get_nvm_settings();

#endif /* APP_LORAWAN_NVM_SETTINGS_H */
