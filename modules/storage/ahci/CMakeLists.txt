if (NOT DEFINED VALI_BUILD)
    cmake_minimum_required(VERSION 3.13.4)
    include(../../../cmake/SetupEnvironment.cmake)
    project(ValiModule_AHCI)
endif ()

enable_language(C)

# Configure include paths
include_directories (
    ${CMAKE_BINARY_DIR}/protocols
    ${CMAKE_CURRENT_BINARY_DIR}
    ../../../librt/libgracht/include
    ../../../librt/libusb/include
    ../../../librt/libddk/include
    ../../../librt/libds/include
    ../../../librt/libc/include
    ../sata
)

set (ADDITONAL_SOURCES
    ${CMAKE_BINARY_DIR}/protocols/ctt_driver_service_server.c
    ${CMAKE_BINARY_DIR}/protocols/ctt_storage_service_server.c
)
set_source_files_properties ( ${ADDITONAL_SOURCES} PROPERTIES GENERATED TRUE )

add_module_target(ahci ""
    ${ADDITONAL_SOURCES}

    ahci.c
    dispatch.c
    main.c
    manager.c
    port.c
    transactions.c
)
add_dependencies(ahci contract_servers)
deploy_file_to_initrd(ahci "modules" ahci.yaml)
