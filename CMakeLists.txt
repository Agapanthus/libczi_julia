cmake_minimum_required(VERSION 3.20)

project(
    libczi_julia
    VERSION 0.2.1
    LANGUAGES C CXX
)

include(GNUInstallDirs)
include(CTest)

set(
    LIBCZI_SOURCE_DIR
    ""
    CACHE PATH
    "Path to a libCZI source checkout"
)

set(
    LIBCZI_JULIA_TEST_FILE
    ""
    CACHE FILEPATH
    "Optional CZI fixture used by the native file smoke test"
)

if(NOT LIBCZI_SOURCE_DIR)
    message(FATAL_ERROR "Set LIBCZI_SOURCE_DIR to a pinned libCZI checkout")
endif()

set(LIBCZI_BUILD_UNITTESTS OFF CACHE BOOL "" FORCE)
set(LIBCZI_BUILD_CZICMD OFF CACHE BOOL "" FORCE)
set(LIBCZI_BUILD_DYNLIB ON CACHE BOOL "" FORCE)
set(LIBCZI_ENABLE_INSTALL ON CACHE BOOL "" FORCE)
set(LIBCZI_BUILD_LIBCZIAPI OFF CACHE BOOL "" FORCE)
set(LIBCZI_BUILD_CURL_BASED_STREAM OFF CACHE BOOL "" FORCE)
set(LIBCZI_BUILD_AZURESDK_BASED_STREAM OFF CACHE BOOL "" FORCE)
set(LIBCZI_BUILD_ENABLE_EXPERIMENTAL_FUNCTIONALITY OFF CACHE STRING "" FORCE)
set(LIBCZI_DO_NOT_SET_MSVC_RUNTIME_LIBRARY ON CACHE BOOL "" FORCE)

add_subdirectory(
    "${LIBCZI_SOURCE_DIR}"
    "${CMAKE_CURRENT_BINARY_DIR}/libczi"
)

add_library(libczi_julia SHARED src/libczi_julia.cpp)
add_library(libczi_julia::libczi_julia ALIAS libczi_julia)

target_compile_features(libczi_julia PRIVATE cxx_std_17)
target_compile_definitions(libczi_julia PRIVATE LIBCZI_JULIA_BUILDING)
target_include_directories(
    libczi_julia
    PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)
target_link_libraries(libczi_julia PRIVATE libCZI)

set_target_properties(
    libczi_julia
    PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN YES
        VERSION "${PROJECT_VERSION}"
        SOVERSION 1
        OUTPUT_NAME czi_julia
)

if(APPLE)
    set_target_properties(
        libczi_julia
        PROPERTIES INSTALL_RPATH "@loader_path"
    )
elseif(UNIX)
    set_target_properties(
        libczi_julia
        PROPERTIES INSTALL_RPATH "$ORIGIN"
    )
endif()

install(
    TARGETS libczi_julia
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)

install(
    FILES include/libczi_julia.h
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)


if(BUILD_TESTING)
    add_executable(libczi_julia_abi_smoke test/abi_smoke.c)
    target_link_libraries(
        libczi_julia_abi_smoke
        PRIVATE libczi_julia
    )
    add_test(NAME libczi_julia_abi_smoke COMMAND libczi_julia_abi_smoke)

    add_executable(libczi_julia_file_smoke test/file_smoke.c)
    target_link_libraries(
        libczi_julia_file_smoke
        PRIVATE libczi_julia
    )

    if(LIBCZI_JULIA_TEST_FILE)
        add_test(
            NAME libczi_julia_file_smoke
            COMMAND
                libczi_julia_file_smoke
                "${LIBCZI_JULIA_TEST_FILE}"
        )
    endif()
endif()
