#
# Copyright (c) 2025, Jamie M.
#
# All right reserved. This code is NOT apache or FOSS/copyleft licensed.
#

ExternalZephyrProject_Add(
  APPLICATION remote
  SOURCE_DIR ${APP_DIR}/remote
  BOARD bl54l15_breakout/nrf54l15/cpuflpr/lora/xip
  BOARD_REVISION ${BOARD_REVISION}
  BUILD_ONLY y
)

set_property(GLOBAL APPEND PROPERTY PM_DOMAINS CPUFLPR)
set_property(GLOBAL APPEND PROPERTY PM_CPUFLPR_IMAGES remote)
set_property(GLOBAL PROPERTY DOMAIN_APP_CPUFLPR remote)
set(CPUFLPR_PM_DOMAIN_DYNAMIC_PARTITION remote CACHE INTERNAL "")

# Add dependency so that the remote image is built/flashed first
sysbuild_add_dependencies(CONFIGURE ${DEFAULT_IMAGE} remote)
sysbuild_add_dependencies(FLASH ${DEFAULT_IMAGE} remote)

# Generated combined hex file
add_custom_command(
  OUTPUT
    ${CMAKE_BINARY_DIR}/combined.hex
  COMMAND
    ${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/scripts/build/mergehex.py
    -o ${CMAKE_BINARY_DIR}/combined.hex
    ${CMAKE_BINARY_DIR}/merged.hex ${CMAKE_BINARY_DIR}/merged_CPUFLPR.hex
  DEPENDS
    ${DEFAULT_IMAGE}_extra_byproducts
    remote_extra_byproducts
)

add_custom_target(
  combined_hex
  ALL
  DEPENDS
    ${CMAKE_BINARY_DIR}/combined.hex
)

add_dependencies(app remote)
