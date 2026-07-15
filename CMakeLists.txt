cmake_minimum_required(VERSION 3.20)
project(libczi_julia VERSION 0.2.5 LANGUAGES C CXX)

include(GNUInstallDirs)
include(CTest)

set(LIBCZI_SOURCE_DIR "" CACHE PATH "Path to a libCZI source checkout")
set(
    LIBCZI_JULIA_TEST_FILE
    ""
    CACHE FILEPATH
    "Optional CZI fixture for the native smoke test"
)
if(NOT IS_DIRECTORY "${LIBCZI_SOURCE_DIR}")
    message(FATAL_ERROR "LIBCZI_SOURCE_DIR must name a libCZI checkout")
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

add_subdirectory("${LIBCZI_SOURCE_DIR}" libczi)

add_library(czi_julia SHARED src/libczi_julia.cpp)
target_compile_features(czi_julia PRIVATE cxx_std_17)
target_compile_definitions(czi_julia PRIVATE LIBCZI_JULIA_BUILDING)
target_include_directories(czi_julia PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(czi_julia PRIVATE libCZI)
set_target_properties(
    czi_julia
    PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN YES
        VERSION "${PROJECT_VERSION}"
        SOVERSION 1
)

if(APPLE)
    set_property(TARGET czi_julia PROPERTY INSTALL_RPATH "@loader_path")
elseif(UNIX)
    set_property(TARGET czi_julia PROPERTY INSTALL_RPATH "$ORIGIN")
endif()

install(
    TARGETS czi_julia
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)
install(
    FILES include/libczi_julia.h
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

if(BUILD_TESTING)
    add_executable(abi_smoke test/abi_smoke.c)
    target_link_libraries(abi_smoke PRIVATE czi_julia)
    add_test(NAME abi_smoke COMMAND abi_smoke)

    if(LIBCZI_JULIA_TEST_FILE)
        add_executable(file_smoke test/file_smoke.c)
        target_link_libraries(file_smoke PRIVATE czi_julia)
        add_test(
            NAME file_smoke
            COMMAND file_smoke "${LIBCZI_JULIA_TEST_FILE}"
        )
    endif()
endif()
