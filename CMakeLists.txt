cmake_minimum_required(VERSION 3.20.0)

set(BOARD qemu_cortex_m3)

find_package(Zephyr)
project(zephyrix)

add_executable(hello src/hello.c)
target_compile_options(hello PRIVATE
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
target_link_options(hello PRIVATE -nostdlib)

add_custom_command(OUTPUT hello.c DEPENDS hello COMMAND xxd -i -n hello $<TARGET_FILE:hello> hello.c)


target_sources(app PRIVATE src/kernel.c hello.c)
