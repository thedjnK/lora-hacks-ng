#
# Copyright (c) 2025, Jamie M.
#
# All right reserved. This code is NOT apache or FOSS/copyleft licensed.
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(test_app_remote)

target_sources(app PRIVATE ../src/main.c ../src/ipc_endpoint.c ../src/ipc_settings.c)
