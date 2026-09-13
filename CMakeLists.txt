cmake_minimum_required(VERSION 3.16)

# Pin the target so a fresh build cannot default to a different ESP32 family.
set(IDF_TARGET "esp32s3")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(picopal)
