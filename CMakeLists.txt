cmake_minimum_required(VERSION 3.16)

# PicoPal 使用 ESP32-S3；先固定 target，避免空白專案被建成其他 ESP32 型號。
set(IDF_TARGET "esp32s3")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(picopal)
