cmake_minimum_required(VERSION 3.25)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CHIMA_VER_MAJ 1)
set(CHIMA_VER_MIN 0)
set(CHIMA_VER_REV 0)

set(CHIMA_VER "${CHIMA_VER_MAJ}.${CHIMA_VER_MIN}.${CHIMA_VER_REV}")

option(CHIMA_EMBED_GIT "Embed git info in version string" ON)
option(CHIMA_SHARED_BUILD "Shared object build" ON)

set(CHIMA_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(CHIMA_SOURCE_DIR "${CHIMA_DIR}/src")
set(CHIMA_INCLUDE_DIR "${CHIMA_DIR}/include")

if (CHIMA_EMBED_GIT)
  execute_process(COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY "${CHIMA_DIR}"
    OUTPUT_VARIABLE CHIMA_GIT_TAG
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  execute_process(COMMAND git rev-list --count HEAD
    WORKING_DIRECTORY "${CHIMA_DIR}"
    OUTPUT_VARIABLE CHIMA_BUILD_ID
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  execute_process(COMMAND git rev-parse --abbrev-ref HEAD
    WORKING_DIRECTORY "${CHIMA_DIR}"
    OUTPUT_VARIABLE CHIMA_GIT_BRANCH
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  set(CHIMA_VER_STR "${CHIMA_VER}-${CHIMA_BUILD_ID}-${CHIMA_GIT_TAG} (${CHIMA_GIT_BRANCH})")
else()
  set(CHIMA_VER_STR "${CHIMA_VER}")
endif()

set(CHIMA_DEFS)
if (CHIMA_SHARED_BUILD)
  list(APPEND CHIMA_DEFS "CHIMA_SHARED_BUILD_")
  set(CHIMA_BUILD_TYPE SHARED)
else()
  set(CHIMA_BUILD_TYPE STATIC)
endif()

message(STATUS "chimatools: Setting up version ${CHIMA_VER_STR}")
project(chimatools VERSION ${CHIMA_VER} LANGUAGES CXX C)

file(GLOB_RECURSE CHIMA_SOURCE_FILES "${CHIMA_SOURCE_DIR}/*.c")
set(CHIMA_INCLUDE_FILES "${CHIMA_INCLUDE_DIR}/chimatools/chimatools.h"
                        "${CHIMA_INCLUDE_DIR}/chimatools/chimatools.hpp")

configure_file("${CHIMA_SOURCE_DIR}/config.h.in" "include/chimatools/config.h" @ONLY)

add_library(chimatools ${CHIMA_BUILD_TYPE})

set_target_properties(chimatools PROPERTIES LINKER_LANGUAGE CXX CXX_STANDARD 17)
target_include_directories(chimatools PUBLIC
                           "$<BUILD_INTERFACE:${CHIMA_INCLUDE_DIR}>"
                           "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")
target_compile_definitions(chimatools PUBLIC ${CHIMA_DEFS})
