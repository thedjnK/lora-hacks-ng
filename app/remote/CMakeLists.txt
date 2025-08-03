#
# Copyright (c) 2025, Jamie M.
#
# All right reserved. This code is NOT apache or FOSS/copyleft licensed.
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(test_app_remote)

target_sources(app PRIVATE ../src/main.c ../src/ipc_endpoint.c ../src/ipc_settings.c)

if(CONFIG_SETTINGS_IPC)
  target_sources(app PRIVATE ../src/settings_ipc.c)
endif()

if(CONFIG_IPC_LORAWAN_CRYPTO_CLIENT)
  target_sources(app PRIVATE ../src/ipc_crypto.c)
endif()

if(CONFIG_LORAWAN_NVM_IPC_SETTINGS)
  set(ZEPHYR_CURRENT_LIBRARY loramac-node)
  zephyr_library_sources(../src/lorawan_nvm_settings.c ../src/lorawan_nvm_settings_setting.c)

  if(CONFIG_LORAWAN_BELL_IPC_CRYPTO_CLIENT)
    zephyr_library_compile_definitions(SOFT_SE)
    zephyr_library_include_directories(${ZEPHYR_LORAMAC_NODE_MODULE_DIR}/src/peripherals/soft-se)

    zephyr_library_sources(
      ${ZEPHYR_LORAMAC_NODE_MODULE_DIR}/src/peripherals/soft-se/aes.c
      ${ZEPHYR_LORAMAC_NODE_MODULE_DIR}/src/peripherals/soft-se/cmac.c
      ../src/soft-se-mod.c
      ${ZEPHYR_LORAMAC_NODE_MODULE_DIR}/src/peripherals/soft-se/soft-se-hal.c
    )
  endif()
endif()
