cmake_minimum_required(VERSION 3.20)

set(CMAKE_SYSTEM_NAME Generic)

set(path ${CMAKE_CURRENT_LIST_DIR}/../../../toolchain/llvm-project/build/bin)

set(CMAKE_ASM_COMPILER ${path}/clang CACHE FILEPATH "")
set(CMAKE_C_COMPILER ${path}/clang CACHE FILEPATH "")
set(CMAKE_CXX_COMPILER ${path}/clang++ CACHE FILEPATH "")
