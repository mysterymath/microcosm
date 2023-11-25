cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr)
project(zephyrix)

set(BOARD qemu_cortex_m3)

target_sources(app PRIVATE src/main.c)
