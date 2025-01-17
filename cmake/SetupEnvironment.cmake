# Make sure all the proper env are set
if(NOT DEFINED ENV{CROSS})
    message(FATAL_ERROR "CROSS environmental variable must point to a clang cross-compiler for Vali")
endif()

if(NOT DEFINED ENV{VALI_ARCH})
    if(VALI_ARCH)
        message(STATUS "VALI_ARCH is set to ${VALI_ARCH}.")
        set(ENV{VALI_ARCH} ${VALI_ARCH})
    else()
        message(STATUS "VALI_ARCH environmental variable was not set, defauling to amd64.")
        set(ENV{VALI_ARCH} amd64)
    endif()
endif()

if (VALI_BOOTSTRAP)
    set (CMAKE_C_COMPILER_WORKS 1)
    set (CMAKE_CXX_COMPILER_WORKS 1)
endif ()

set (CMAKE_SYSTEM_NAME valicc)
set (CMAKE_C_COMPILER "$ENV{CROSS}/bin/clang" CACHE FILEPATH "")
set (CMAKE_CXX_COMPILER "$ENV{CROSS}/bin/clang++" CACHE FILEPATH "")
set (CMAKE_AR "$ENV{CROSS}/bin/llvm-ar" CACHE FILEPATH "")
set (CMAKE_RANLIB "$ENV{CROSS}/bin/llvm-ranlib" CACHE FILEPATH "")
set (VERBOSE 1)

# Setup the default for the linker to create .map files so we can debug
set (CMAKE_SHARED_LINKER_FLAGS "-Xlinker -lldmap")
set (CMAKE_EXE_LINKER_FLAGS "-Xlinker -lldmap")

# Setup shared compile flags to make compilation succeed
# -Xclang -flto-visibility-public-std
set(VALI_COMPILE_FLAGS -fms-extensions -nostdlib -nostdinc)
if("$ENV{VALI_ARCH}" STREQUAL "i386")
    set(VALI_COMPILE_FLAGS ${VALI_COMPILE_FLAGS} --target=i386-uml-vali)
elseif("$ENV{VALI_ARCH}" STREQUAL "amd64")
    set(VALI_COMPILE_FLAGS ${VALI_COMPILE_FLAGS} --target=amd64-uml-vali)
endif()
string(REPLACE ";" " " VALI_COMPILE_FLAGS "${VALI_COMPILE_FLAGS}")

# We need to preserve any flags that were passed in by the user. However, we
# can't append to CMAKE_C_FLAGS and friends directly, because toolchain files
# will be re-invoked on each reconfigure and therefore need to be idempotent.
# The assignments to the _INITIAL cache variables don't use FORCE, so they'll
# only be populated on the initial configure, and their values won't change
# afterward.
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${VALI_COMPILE_FLAGS}" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${VALI_COMPILE_FLAGS}" CACHE STRING "" FORCE)