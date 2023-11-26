cmake_minimum_required(VERSION 3.20.0)

set(BOARD qemu_cortex_m3)

find_package(Zephyr)
project(zephyrix)

add_executable(hello src/hello.c)
set(USERSPACE_COMPILE_OPTIONS
  -Os
  -mcpu=cortex-m3
  -mthumb
  -mabi=aapcs
  -mfp16-format=ieee
  -mtp=soft
  -fpie
  -fno-asynchronous-unwind-tables
  -ftls-model=local-exec
  --param=min-pagesize=0
  -ffunction-sections
  -fdata-sections
  -std=c99
)
set(USERSPACE_LINK_OPTIONS
  -Wl,-z,max-page-size=1
  -Wl,-z,common-page-size=1
  -Ttext=0
  -nostdlib
)

target_compile_options(hello PRIVATE ${USERSPACE_COMPILE_OPTIONS})
target_link_options(hello PRIVATE
  ${USERSPACE_COMPILE_OPTIONS}
  ${USERSPACE_LINK_OPTIONS}
)

target_sources(app PRIVATE src/kernel.c src/hello.S)
target_compile_definitions(app PRIVATE HELLO_FILE=\"$<TARGET_FILE:hello>\")
add_dependencies(app hello)
