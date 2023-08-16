cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Include SPSP
list(APPEND EXTRA_COMPONENT_DIRS "PATH/TO/spsp/spsp-esp-idf")

project(spsp_client_example)
