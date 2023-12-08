cmake_minimum_required(VERSION 3.20.0)

set(BOARD qemu_cortex_m0)

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
    -DCMAKE_C_FLAGS=-mcpu=cortex-m0
  DOWNLOAD_COMMAND ""
  UPDATE_COMMAND ""
  BUILD_ALWAYS On
)

function(generate_userland_inc generated_file source) 
  set(dst ${ZEPHYR_BINARY_DIR}/include/generated/${generated_file})
  set(src ${CMAKE_CURRENT_BINARY_DIR}/userland-prefix/bin/${source})

  add_custom_command(
    OUTPUT ${dst}
    COMMAND
      ${PYTHON_EXECUTABLE}
      ${ZEPHYR_BASE}/scripts/build/file2hex.py
      ${ARGN} # Extra arguments are passed to file2hex.py
      --file ${src}
      > ${dst} # Does pipe redirection work on Windows?
    DEPENDS
      userland
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )
  generate_unique_target_name_from_filename(${dst} generated_target_name)
  add_custom_target(${generated_target_name} DEPENDS ${dst})
  add_dependencies(app ${generated_target_name})
endfunction()

generate_userland_inc(hello.inc hello)
target_sources(app PRIVATE src/kernel.c)
add_dependencies(app userland)
