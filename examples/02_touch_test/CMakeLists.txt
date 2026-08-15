cmake_minimum_required(VERSION 3.16)

set(SDKCONFIG_DEFAULTS
    "${CMAKE_CURRENT_LIST_DIR}/../../software/espressif/config/panlee-v15-230208-sample-a.idf6.0.2.sdkconfig.defaults")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(wt32_sc01_plus_touch_test)
