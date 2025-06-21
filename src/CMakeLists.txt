cmake_minimum_required(VERSION 3.21.7)
project(libczi_julia LANGUAGES CXX)
set(JL_TARGET czi_julia)

# ---------------- options ----------------
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_INSTALL_LIBDIR "lib")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# -------------- dependencies -------------
set(LIBCZI_BUILD_UNITTESTS OFF CACHE BOOL "" FORCE)
set(LIBCZI_BUILD_DYNLIB OFF CACHE BOOL "" FORCE)
set(LIBCZI_BUILD_CZICMD OFF CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../libczi)
  
# ------------- wrapper library ----------
add_library(${JL_TARGET} SHARED libczi_wrap.cpp)
target_include_directories(${JL_TARGET} PRIVATE ${libCZI_SOURCE_DIR}/Src)
target_link_libraries(${JL_TARGET} PRIVATE libCZI::libCZI)

install(TARGETS
  ${JL_TARGET}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)