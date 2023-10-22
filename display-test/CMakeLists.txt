cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(display-test)

target_include_directories(app PRIVATE ${CMAKE_BINARY_DIR}/app/include include)

FILE(GLOB_RECURSE app_sources CONFIGURE_DEPENDS src/*.c)
target_sources(app PRIVATE ${app_sources})
