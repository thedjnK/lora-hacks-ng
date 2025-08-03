/*
 * Copyright (c) 2022 Intellinium <giuliano.franchetto@intellinium.com>
 * Copyright (c) 2025, Jamie M.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <LoRaMac.h>
#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include <zephyr/logging/log.h>
#include "lorawan_nvm_settings.h"

LOG_MODULE_REGISTER(lorawan_nvm, CONFIG_LORAWAN_LOG_LEVEL);

#define NVM_SETTING_VALUE_DESCR(_name) \
	static uint8_t setting_value_ ## _name[sizeof(((LoRaMacNvmData_t *)0)->_name)]; \
	static bool setting_loaded_ ## _name

#define NVM_SETTING_DESCR(_flag, _member)				\
	{								\
		.flag = _flag,						\
		.name = STRINGIFY(_member),				\
		.setting_name =						\
			LORAWAN_SETTINGS_BASE "/" STRINGIFY(_member),	\
		.offset = offsetof(LoRaMacNvmData_t, _member),		\
		.size = sizeof(((LoRaMacNvmData_t *)0)->_member),	\
		.data = setting_value_ ## _member,			\
		.loaded = &setting_loaded_ ## _member,			\
	}

NVM_SETTING_VALUE_DESCR(Crypto);
NVM_SETTING_VALUE_DESCR(MacGroup1);
NVM_SETTING_VALUE_DESCR(MacGroup2);
NVM_SETTING_VALUE_DESCR(SecureElement);
NVM_SETTING_VALUE_DESCR(RegionGroup2);

static const struct lorawan_nvm_setting_descr nvm_setting_descriptors[] = {
	NVM_SETTING_DESCR(LORAMAC_NVM_NOTIFY_FLAG_CRYPTO, Crypto),
	NVM_SETTING_DESCR(LORAMAC_NVM_NOTIFY_FLAG_MAC_GROUP1, MacGroup1),
	NVM_SETTING_DESCR(LORAMAC_NVM_NOTIFY_FLAG_MAC_GROUP2, MacGroup2),
	NVM_SETTING_DESCR(LORAMAC_NVM_NOTIFY_FLAG_SECURE_ELEMENT, SecureElement),
	NVM_SETTING_DESCR(LORAMAC_NVM_NOTIFY_FLAG_REGION_GROUP2, RegionGroup2),
};

const uint16_t lorawan_nvm_settings_entries = ARRAY_SIZE(nvm_setting_descriptors);

static void lorawan_nvm_save_settings(uint16_t nvm_notify_flag)
{
	MibRequestConfirm_t mib_req;
	LoRaMacNvmData_t *nvm;

	LOG_DBG("Saving LoRaWAN settings");

	/* Retrieve the actual context */
	mib_req.Type = MIB_NVM_CTXS;

	if (LoRaMacMibGetRequestConfirm(&mib_req) != LORAMAC_STATUS_OK) {
		LOG_ERR("Could not get NVM context");
		return;
	}

	nvm = mib_req.Param.Contexts;

	LOG_DBG("Crypto version: %"PRIu32", DevNonce: %d, JoinNonce: %"PRIu32,
		mib_req.Param.Contexts->Crypto.LrWanVersion.Value,
		mib_req.Param.Contexts->Crypto.DevNonce,
		mib_req.Param.Contexts->Crypto.JoinNonce);

	for (uint32_t i = 0; i < ARRAY_SIZE(nvm_setting_descriptors); i++) {
		const struct lorawan_nvm_setting_descr *descr =
			&nvm_setting_descriptors[i];

		if ((nvm_notify_flag & descr->flag) == descr->flag) {
			int rc;

			LOG_DBG("Saving configuration " LORAWAN_SETTINGS_BASE "/%s", descr->name);
			rc = settings_save_one(descr->setting_name, (char *)nvm + descr->offset,
					       descr->size);

			if (rc != 0) {
				LOG_ERR("Could not save setting " LORAWAN_SETTINGS_BASE "/%s: %d",
					descr->name, rc);
			}
		}
	}
}

void lorawan_nvm_data_mgmt_event(uint16_t flags)
{
	if (flags != LORAMAC_NVM_NOTIFY_FLAG_NONE) {
		lorawan_nvm_save_settings(flags);
	}
}

int lorawan_nvm_data_restore(void)
{
	int err;
	LoRaMacStatus_t status;
	MibRequestConfirm_t mib_req;

	LOG_DBG("Restoring LoRaWAN settings");

	/* Retrieve the actual context */
	mib_req.Type = MIB_NVM_CTXS;

	if (LoRaMacMibGetRequestConfirm(&mib_req) != LORAMAC_STATUS_OK) {
		LOG_ERR("Could not get NVM context");
		return -EINVAL;
	}


	for (uint32_t i = 0; i < ARRAY_SIZE(nvm_setting_descriptors); i++) {
		const struct lorawan_nvm_setting_descr *descr =
			&nvm_setting_descriptors[i];

		if (*descr->loaded == true) {
LOG_ERR("value %s to %p from %p", descr->name, (void *)((char *)mib_req.Param.Contexts + descr->offset), (void *)descr->data);
			memcpy(((char *)mib_req.Param.Contexts + descr->offset), descr->data, descr->size);
LOG_HEXDUMP_ERR(descr->data, descr->size, "data");
		}
	}

	LOG_DBG("Crypto version: %"PRIu32", DevNonce: %d, JoinNonce: %"PRIu32,
		mib_req.Param.Contexts->Crypto.LrWanVersion.Value,
		mib_req.Param.Contexts->Crypto.DevNonce,
		mib_req.Param.Contexts->Crypto.JoinNonce);

	mib_req.Type = MIB_NVM_CTXS;
	status = LoRaMacMibSetRequestConfirm(&mib_req);

	if (status != LORAMAC_STATUS_OK) {
		LOG_ERR("Could not set the NVM context, error %d", status);
		return -EINVAL;
	}

	LOG_DBG("LoRaWAN context restored");

	return 0;
}

int lorawan_nvm_init(void)
{
}

const struct lorawan_nvm_setting_descr *lorawan_get_nvm_settings()
{
	return nvm_setting_descriptors;
}
