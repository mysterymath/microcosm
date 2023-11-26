cmake_minimum_required(VERSION 3.20.0)

set(BOARD qemu_cortex_m3)

find_package(Zephyr)
project(microcosm)

set_property(GLOBAL PROPERTY TARGET_SUPPORTS_SHARED_LIBS TRUE)

include(ExternalProject)
ExternalProject_Add(userland
  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/userland
  CMAKE_ARGS
    -DCMAKE_TOOLCHAIN_FILE=${CMAKE_CURRENT_SOURCE_DIR}/userland/cmake/toolchain.cmake
    -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
    -DCMAKE_BUILD_TYPE=MinSizeRel
    -DCMAKE_C_FLAGS=-mcpu=cortex-m3
  DOWNLOAD_COMMAND ""
  UPDATE_COMMAND ""
)

generate_inc_file_for_target(app ${CMAKE_CURRENT_BINARY_DIR}/userland-prefix/bin/hello ${ZEPHYR_BINARY_DIR}/include/generated/hello.inc)
target_sources(app PRIVATE src/kernel.c)
add_dependencies(app userland)
