# handle all unit test libraries that should be built for host
if (VALI_BUILD_UNIT_TESTS)
    add_subdirectory (libds)
    return ()
endif ()

string(TOLOWER "${CMAKE_C_COMPILER_ID}-${CMAKE_C_COMPILER_VERSION}" VALI_COMPILER_ID)

macro (merge_libraries)
    # To produce a library we need at least one source file.
    # It is created by ADD_CUSTOM_COMMAND below and will helps 
    # also help to track dependencies.
    set (TARGET ${ARGV0})
    set (LIB_LIST "${ARGN}")
    list (REMOVE_AT LIB_LIST 0)

    set (SOURCE_FILE ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}_depends.c)
    add_library (${TARGET} STATIC ${SOURCE_FILE})

    # Get a list of libraries and their full path
    foreach (LIB ${LIB_LIST})
        if (TARGET ${LIB})
            set(LIB_LOCATION ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}/${LIB}.lib)
            
            set (STATIC_LIBS ${STATIC_LIBS} ${LIB_LOCATION})
            add_dependencies(${TARGET} ${LIB})
        else ()
            list (APPEND STATIC_LIBS ${LIB}.lib)
        endif ()
    endforeach ()
    
    # Make the generated dummy source file depended on all static input
    # libs. If input lib changes,the source file is touched
    # which causes the desired effect (relink).
    add_custom_command ( 
        OUTPUT  ${SOURCE_FILE}
        COMMAND ${CMAKE_COMMAND} -E touch ${SOURCE_FILE}
        DEPENDS ${STATIC_LIBS}
    )

    if (MSVC)
        # To merge libs, just pass them to lib.exe command line.
        set (LINKER_EXTRA_FLAGS "")
        foreach (LIB ${STATIC_LIBS})
            set (LINKER_EXTRA_FLAGS "${LINKER_EXTRA_FLAGS} ${LIB}")
        endforeach ()
        set_target_properties (${TARGET}
            PROPERTIES
                STATIC_LIBRARY_FLAGS "${LINKER_EXTRA_FLAGS}")
    else ()
        set (TARGET_LOCATION ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}/${TARGET}.lib)
        if (APPLE)
          # Use OSX's libtool to merge archives (ihandles universal 
          # binaries properly)
            add_custom_command (TARGET ${TARGET} POST_BUILD
                COMMAND rm ${TARGET_LOCATION}
                COMMAND /usr/bin/libtool -static -o ${TARGET_LOCATION} 
                ${STATIC_LIBS}
            )
        else ()
            # Generic Unix, Cygwin or MinGW. In post-build step, call
            # script, that extracts objects from archives with "ar x" 
            # and repacks them with "ar r"
            set (TARGET ${TARGET})
            configure_file (
                ../cmake/MergeLibraries.cmake.in
                ${CMAKE_CURRENT_BINARY_DIR}/merge_archives_${TARGET}.cmake 
                @ONLY
            )
            add_custom_command (TARGET ${TARGET} POST_BUILD
              COMMAND rm ${TARGET_LOCATION}
              COMMAND ${CMAKE_COMMAND} -P 
              ${CMAKE_CURRENT_BINARY_DIR}/merge_archives_${TARGET}.cmake
            )
        endif ()
    endif ()
endmacro ()

# COMPILER-RT
include (ExternalProject)
ExternalProject_Add(compiler-rt
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/compiler-rt
    CMAKE_ARGS
        -DCMAKE_TOOLCHAIN_FILE=${CMAKE_CURRENT_SOURCE_DIR}/../cmake/ToolchainFile.cmake
        -DCMAKE_C_FLAGS=${VALI_COMPILER_RT_C_FLAGS}
        -DCMAKE_CXX_FLAGS=${VALI_COMPILER_RT_CXX_FLAGS}
        -DCMAKE_ASM_FLAGS=${VALI_COMPILER_RT_ASM_FLAGS}
        -DCOMPILER_RT_STANDALONE_BUILD=ON
        -DCOMPILER_RT_BUILD_BUILTINS=ON
        -DCOMPILER_RT_BUILD_SANITIZERS=OFF
        -DCOMPILER_RT_BUILD_XRAY=OFF
        -DCOMPILER_RT_BUILD_LIBFUZZER=OFF
        -DCOMPILER_RT_BUILD_PROFILE=OFF
        -DCOMPILER_RT_BAREMETAL_BUILD=ON
        -DCOMPILER_RT_EXCLUDE_ATOMIC_BUILTIN=OFF
        -DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY
        -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_BINARY_DIR}
        -DVALI_ARCH=${VALI_ARCH}
)

LINK_DIRECTORIES(${CMAKE_BINARY_DIR}/lib/valicc)

# Runtime libraries
add_subdirectory (libcrt)
add_subdirectory (libds)
add_subdirectory (libddk)
add_subdirectory (libos)

merge_libraries (librt libcrt ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/valicc/${VALI_COMPILER_RT_TARGET})
add_dependencies (librt compiler-rt)
install (TARGETS librt
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

# Libraries used by building of everything
add_subdirectory (libacpi)

set (GRACHT_C_LINK_VALI ON CACHE BOOL "Enable vali ipc link")
add_subdirectory (libgracht)
add_subdirectory (libusb)

# Math Standard Library for Vali
add_subdirectory (libm)

# C Standard Library for Vali
add_subdirectory (libc)

# Libraries that depend on the c lib
add_subdirectory(libyaml)

# build zstd in the static flavor for now. To build the shared one
# we need to fix a few issues in our cmake platform setup
set(ZSTD_LEGACY_SUPPORT OFF CACHE BOOL "Disable legacy support for zstd")
set(ZSTD_BUILD_PROGRAMS OFF CACHE BOOL "Do not build zstd programs")
set(ZSTD_BUILD_CONTRIB  OFF CACHE BOOL "Do not build zstd contrib")
set(ZSTD_BUILD_TESTS    OFF CACHE BOOL "Do not build zstd tests")
set(ZSTD_BUILD_SHARED   OFF CACHE BOOL "Do not build shared library")
add_subdirectory(zstd/build/cmake)

# Unfortunately we have to manually add include paths to the targets
# introduced by zstd, as we are compiling free-standing at this moment
target_link_libraries(libzstd_static PRIVATE libos-api c)

# create an interface library for zstd
add_library(zstd-api INTERFACE)
target_include_directories(zstd-api INTERFACE zstd/lib)

# Export all the ddk targets for easier use
install(EXPORT ddk_targets
    FILE ValiDDKTargets.cmake
    NAMESPACE ValiDDK::
    DESTINATION lib/cmake
)
