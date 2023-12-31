#
# Copyright(c) 2019 to 2022 ZettaScale Technology and others
#
# This program and the accompanying materials are made available under the
# terms of the Eclipse Public License v. 2.0 which is available at
# http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
# v. 1.0 which is available at
# http://www.eclipse.org/org/documents/edl-v10.php.
#
# SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause
#
project(ddsVideoTrans LANGUAGES C)
cmake_minimum_required(VERSION 3.16)

# set(ENV{KernelSoftDDS_DIR} /home/dev/code/cyclonedds/install)
# set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH};/home/dev/code/cyclonedds/install")

set(CMAKE_BUILD_TYPE Debug)

# Find the KernelSoftDDS package.
# find_package(KernelSoftDDS REQUIRED)

# find_package(PkgConfig REQUIRED)

# pkg_check_modules(FFMPEG REQUIRED libavformat libavutil libavcodec)
# pkg_check_modules(LIBAV REQUIRED IMPORTED_TARGET
#       libavdevice
#       libavfilter
#       libavformat
#       libavcodec
#       libswresample
#       libswscale
#       libavutil
# )

# idlc_generate(TARGET VideoData_lib FILES "VideoData.idl" WARNINGS no-implicit-extensibility)
# This is a convenience function, provided by the KernelSoftDDS package,
# that will supply a library target related the the given idl file.
# In short, it takes the idl file, generates the source files with
# the proper data types and compiles them into a library.
# idlc_generate(TARGET HelloWorldData_lib FILES "HelloWorldData.idl" WARNINGS no-implicit-extensibility)

include_directories(${PROJECT_SOURCE_DIR})

# Both executables have only one related source file.
# add_executable(videoPub publisher.c)
# add_executable(videoSub subscriber.c)
add_executable(avl_test avl_test.c avl.c)
add_executable(rbt_test rbt_test.c rbtree.c)

# Both executables need to be linked to the idl data type library and
# the ddsc API library.
# target_link_libraries(videoPub VideoData_lib KernelSoftDDS::ddsc PkgConfig::LIBAV)
# target_link_libraries(videoSub VideoData_lib KernelSoftDDS::ddsc PkgConfig::LIBAV)
