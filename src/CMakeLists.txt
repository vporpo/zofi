# Cmake configuration file.
#
# Copyright (C) 2019 Vasileios Porpodas <v.porpodas at gmail.com>
#
# This file is part of ZOFI.
#
# ZOFI is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free
# Software Foundation; either version 2, or (at your option) any later
# version.
# GCC is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
# You should have received a copy of the GNU General Public License
# along with GCC; see the file LICENSE.  If not see
# <http://www.gnu.org/licenses/>.

cmake_minimum_required (VERSION 3.1)
project (zofi)

file(GLOB SOURCES *.cpp *.h *.def)
add_executable(zofi ${SOURCES})

set(BIN_NAME \"zofi\")
set(VERSION \"0.9.7\")
configure_file (
  "${PROJECT_SOURCE_DIR}/config.h.in"
  "${PROJECT_BINARY_DIR}/config.h"
  )

include_directories("${PROJECT_SOURCE_DIR}/")
include_directories("${PROJECT_BINARY_DIR}/")
include_directories(SYSTEM "capstone/capstone.h")

find_library(CAPSTONE_LIB capstone)
find_library(MATH_LIB m)
find_library(PTHREAD_LIB pthread)
find_library(UTIL_LIB util)
target_link_libraries(zofi ${CAPSTONE_LIB} ${MATH_LIB} ${PTHREAD_LIB} ${UTIL_LIB})

set_property(TARGET zofi PROPERTY CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")
set_property(TARGET zofi PROPERTY CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall")

set_property(TARGET zofi PROPERTY CXX_STANDARD 14)
set_property(TARGET zofi PROPERTY CXX_STANDARD_REQUIRED ON)

set(TEST_DIR ${PROJECT_SOURCE_DIR}/../test/)
set(ZIT_DIR ${TEST_DIR}/zit_tests)
add_custom_target(check COMMAND ${TEST_DIR}/zit -s ${ZIT_DIR}/*.c ${ZIT_DIR}/*.cpp)

install(TARGETS zofi DESTINATION /usr/local/bin/)
