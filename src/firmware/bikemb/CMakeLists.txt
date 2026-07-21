cmake_minimum_required(VERSION 3.16)

list(APPEND EXTRA_COMPONENT_DIRS
    "${CMAKE_CURRENT_LIST_DIR}/src"
    "${CMAKE_CURRENT_LIST_DIR}/../../build/pio-libdeps/esp32-s3-touch-lcd-1-85c-idf/lvgl"
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(bikemb)

idf_component_get_property(esp_lcd_lib esp_lcd COMPONENT_LIB)
target_compile_options(${esp_lcd_lib} PRIVATE -O0)
