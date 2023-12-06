cmake_minimum_required(VERSION 3.20.0)

set(CMAKE_MODULE_PATH CACHE ${CMAKE_CURRENT_LIST_DIR}/cmake STRING "")

project(userland)

add_library(kernel-stub SHARED kernel-stub.c)

add_executable(hello hello.c)
target_link_libraries(hello kernel-stub)
install(TARGETS hello)
