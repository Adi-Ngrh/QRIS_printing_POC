# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.22)

set(EXTRA_COMPONENT_DIRS ${CMAKE_CURRENT_LIST_DIR}/connection_config ${CMAKE_CURRENT_LIST_DIR}/printer_code) #[[add subfolder into project]]

include($ENV{IDF_PATH}/tools/cmake/project.cmake) #[[run ESP-IDF component discovery. put after all subfolders addition]]
project(qris_printing_poc)
