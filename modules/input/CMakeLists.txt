if (NOT DEFINED VALI_BUILD)
    cmake_minimum_required(VERSION 3.13.4)
    include(../../cmake/SetupEnvironment.cmake)
    project(ValiStorageModules)
endif ()

# build storage modules
add_subdirectory(hid)
