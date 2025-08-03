#
# Copyright (c) 2025, Jamie M.
#
# All right reserved. This code is NOT apache or FOSS/copyleft licensed.
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(test_app_main)

target_sources(app PRIVATE src/main.c src/ipc_endpoint.c src/flipper_cpu.c src/ipc_settings.c src/ipc_crypto.c)

zephyr_include_directories(${ZEPHYR_BINARY_DIR}/include)
generate_inc_file_for_target(
  app
  ${ZEPHYR_BINARY_DIR}/../../remote/zephyr/zephyr.bin
  ${ZEPHYR_BINARY_DIR}/include/generated/remote.inc
  )

zephyr_linker_sources(ROM_START flipper_code.ld)
