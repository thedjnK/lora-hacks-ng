/*
 * Copyright (c) 2025, Jamie M.
 *
 * All right reserved. This code is NOT apache or FOSS/copyleft licensed.
 */

#include <stdint.h>

int ipc_setting_save(uint8_t *name, uint8_t *value, uint8_t value_size);
int ipc_setting_load(uint8_t *name, uint8_t *value, uint8_t max_value_size);
int ipc_setting_commit(void);
int ipc_setting_tree_count(uint16_t *count);
