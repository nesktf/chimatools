cmake_minimum_required(VERSION 3.25)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CHIMA_VER_MAJ 1)
set(CHIMA_VER_MIN 0)
set(CHIMA_VER_REV 0)

set(CHIMA_VER "${CHIMA_VER_MAJ}.${CHIMA_VER_MIN}.${CHIMA_VER_REV}")

option(CHIMA_EMBED_GIT "Embed git info in version string" ON)
option(CHIMA_SHARED_BUILD "Shared object build" OFF)
option(CHIMA_BUILD_EXAMPLES "Build chimatools examples" OFF)

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

if (CHIMA_SHARED_BUILD)
  set(CHIMA_BUILD_TYPE SHARED)
else()
  set(CHIMA_BUILD_TYPE STATIC)
endif()

message(STATUS "chimatools: Setting up version ${CHIMA_VER_STR}")
project(chimatools VERSION ${CHIMA_VER} LANGUAGES CXX C)

file(GLOB_RECURSE CHIMA_SOURCE_FILES "${CHIMA_SOURCE_DIR}/*.c")
set(CHIMA_INCLUDE_FILES "${CHIMA_INCLUDE_DIR}/chimatools/chimatools.h"
                        "${CHIMA_INCLUDE_DIR}/chimatools/chimatools.hpp")

add_library(${PROJECT_NAME} ${CHIMA_BUILD_TYPE})
target_sources(${PROJECT_NAME} PRIVATE ${CHIMA_SOURCE_FILES})
target_link_libraries(${PROJECT_NAME} -lm)

if (CHIMA_SHARED_BUILD)
  target_compile_definitions(${PROJECT_NAME} PRIVATE -DCHIMA_SHARED_BUILD_)
endif()
set_target_properties(${PROJECT_NAME} PROPERTIES DEBUG_POSTFIX "d")

target_include_directories(${PROJECT_NAME} PUBLIC
                           "$<BUILD_INTERFACE:${CHIMA_INCLUDE_DIR}>"
                           "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")

include(GNUInstallDirs)

install(TARGETS ${PROJECT_NAME}
        EXPORT ${PROJECT_NAME}-config
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(EXPORT ${PROJECT_NAME}-config
        NAMESPACE ${PROJECT_NAME}::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})

install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/include/${PROJECT_NAME}/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME})

install(FILES ${CMAKE_CURRENT_LIST_DIR}/xdg/chima-asset.xml
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/mime/packages)

if (CHIMA_BUILD_EXAMPLES)
  message(STATUS "chimatools: Examples enabled")
  add_subdirectory("${CHIMA_DIR}/examples")
endif()
