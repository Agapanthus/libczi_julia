cmake_minimum_required(VERSION 3.21.7)
project(libczi_julia LANGUAGES CXX)
set(JL_TARGET czi_julia)

# ---------------- options ----------------
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# to avoid lib64 dirs as binarybuilder uses lib everywhere
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_INSTALL_LIBDIR "lib")

# -------------- dependencies -------------
include(FetchContent)

# Disable libczi components we don't need
add_compile_definitions(_LIBCZISTATICLIB)
add_compile_definitions(LIBCZI_BUILD_UNITTESTS=OFF)
add_compile_definitions(LIBCZI_BUILD_DYNLIB=OFF)
add_compile_definitions(LIBCZI_BUILD_CZICMD=OFF)

FetchContent_Declare(
  libCZI
  GIT_REPOSITORY https://github.com/Agapanthus/libczi.git
  GIT_TAG        62f721e577ef7ba2135383e65d83015b6b834db3  
  CMAKE_ARGS -DCMAKE_CXX_STANDARD=17
)
FetchContent_MakeAvailable(libCZI)
set(LIBCZI_TARGET libCZI)

# Find CxxWrap (JlCxx provides cxxwrap_julia)
find_package(JlCxx REQUIRED)
get_target_property(JlCxx_location JlCxx::cxxwrap_julia LOCATION)
get_filename_component(JlCxx_location ${JlCxx_location} DIRECTORY)
message(STATUS "Found JlCxx at ${JlCxx_location}")

# Install RPATH for runtime linkage
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib;${JlCxx_location}")

# ------------- wrapper library ----------
add_library(${JL_TARGET} SHARED libczi_wrap.cpp)
target_include_directories(${JL_TARGET} PRIVATE ${libCZI_SOURCE_DIR}/Src)
target_link_libraries(${JL_TARGET} PRIVATE ${LIBCZI_TARGET} JlCxx::cxxwrap_julia  JlCxx::cxxwrap_julia_stl)

install(TARGETS
  ${JL_TARGET}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)