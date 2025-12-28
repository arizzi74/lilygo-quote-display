# Minimum CMake version required by ESP-IDF
cmake_minimum_required(VERSION 3.16)

# Include ESP-IDF build system
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Define the project
project(t5_epd_hello_world)
