include(FetchContent)
FetchContent_Declare(
        cmocka
        GIT_REPOSITORY https://gitlab.com/cmocka/cmocka.git
        GIT_TAG        f5e2cd77c88d9f792562888d2b70c5a396bfbf7a #cmocka-1.1.5
)
set(WITH_STATIC_LIB OFF)
set(WITH_CMOCKERY_SUPPORT OFF)
set(WITH_EXAMPLES OFF)
set(UNIT_TESTING OFF)
set(PICKY_DEVELOPER OFF)
FetchContent_MakeAvailable(cmocka)
