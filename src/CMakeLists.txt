cmake_minimum_required(VERSION 3.22)
project(libczi_julia LANGUAGES CXX)
set(JL_TARGET libczi_julia)

# ---------------- options ----------------
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_INSTALL_LIBDIR "lib")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH ON)

# -------------- dependencies -------------
include(FetchContent)

add_compile_definitions(_LIBCZISTATICLIB)
add_compile_definitions(LIBCZI_BUILD_UNITTESTS=OFF)
add_compile_definitions(LIBCZI_BUILD_DYNLIB=OFF)
add_compile_definitions(LIBCZI_BUILD_CZICMD=OFF)
FetchContent_Declare(
  libCZI
  GIT_REPOSITORY https://github.com/ZEISS/libczi.git
  # pin a specific commit
  GIT_TAG        543d8025609fa94df2c4131568a50fc6100c0ca2  # as of 2025-05-31
)
FetchContent_MakeAvailable(libCZI)
set(LIBCZI_TARGET libCZI)

find_package(JlCxx REQUIRED)
get_target_property(JlCxx_location JlCxx::cxxwrap_julia LOCATION)
get_filename_component(JlCxx_location ${JlCxx_location} DIRECTORY)
message(STATUS "Found JlCxx at ${JlCxx_location}")

set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib;${JlCxx_location}")

# ------------- wrapper library ----------

add_library(${JL_TARGET} MODULE libczi_wrap.cpp)
target_include_directories(${JL_TARGET} PRIVATE ${libCZI_SOURCE_DIR}/Src)
target_link_libraries(${JL_TARGET} PRIVATE ${LIBCZI_TARGET} JlCxx::cxxwrap_julia)

install(TARGETS
  ${JL_TARGET}
ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
