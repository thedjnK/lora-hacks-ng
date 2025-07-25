#
# Copyright (c) 2023 Nordic Semiconductor
# Copyright (c) 2025, Jamie M.
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

include(${ZEPHYR_NRF_MODULE_DIR}/cmake/extensions.cmake)

# Function to update an already generated runners file for a Zephyr image.
# This function allows the build system to adjust files to flash in the config
# section of the runners.yaml on a given image.
function(update_runner)
  cmake_parse_arguments(RUNNER "" "IMAGE;HEX;BIN;ELF" "" ${ARGN})

  check_arguments_required("update_runner" RUNNER IMAGE)

  get_target_property(bin_dir ${RUNNER_IMAGE} _EP_BINARY_DIR)
  set(runners_file ${bin_dir}/zephyr/runners.yaml)

  set(runners_content_update)
  file(STRINGS ${runners_file} runners_content)
  foreach(line IN LISTS runners_content)
    if(DEFINED RUNNER_ELF AND "${line}" MATCHES "^.*elf_file: .*$")
      string(REGEX REPLACE "(.*elf_file:) .*" "\\1 ${RUNNER_ELF}" line ${line})
      set(${RUNNER_IMAGE}_NCS_RUNNER_ELF "${RUNNER_ELF}" CACHE INTERNAL
          "nRF Connect SDK partition managere controlled elf file"
      )
    endif()

    if(DEFINED RUNNER_HEX AND "${line}" MATCHES "^.*hex_file: .*$")
      string(REGEX REPLACE "(.*hex_file:) .*" "\\1 ${RUNNER_HEX}" line ${line})
      set(${RUNNER_IMAGE}_NCS_RUNNER_HEX "${RUNNER_HEX}" CACHE INTERNAL
          "nRF Connect SDK partition managere controlled hex file"
      )
    endif()

    if(DEFINED RUNNER_BIN AND "${line}" MATCHES "^.*bin_file: .*$")
      string(REGEX REPLACE "(.*bin_file:) .*" "\\1 ${RUNNER_BIN}" line ${line})
      set(${RUNNER_IMAGE}_NCS_RUNNER_BIN "${RUNNER_BIN}" CACHE INTERNAL
          "nRF Connect SDK partition managere controlled bin file"
      )
    endif()
    list(APPEND runners_content_update "${line}\n")
  endforeach()
  file(WRITE ${runners_file} ${runners_content_update})

  # NCS has updated the cache with an NCS_RUNNER file, thus re-create the sysbuild cache.
  # No need for CMAKE_RERUN in this case, as runners.yaml has been updated above.
  sysbuild_cache(CREATE APPLICATION ${RUNNER_IMAGE})
endfunction()

function(${SYSBUILD_CURRENT_MODULE_NAME}_post_cmake)
  cmake_parse_arguments(POST_CMAKE "" "" "IMAGES" ${ARGN})

  if(SB_CONFIG_APP_USE_COMBINED_HEX_FILE)
    update_runner(IMAGE ${DEFAULT_IMAGE} HEX ${CMAKE_BINARY_DIR}/combined.hex)
  endif()
endfunction()
