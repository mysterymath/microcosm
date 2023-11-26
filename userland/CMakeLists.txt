cmake_minimum_required(VERSION 3.20.0)

project(userland)

add_executable(hello hello.c)
install(TARGETS hello)
