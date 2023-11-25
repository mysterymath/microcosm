cmake_minimum_required(VERSION 3.20.0)

set(BOARD qemu_cortex_m3)

find_package(Zephyr)
project(zephyrix)

target_sources(app PRIVATE src/main.c)
