include_guard(GLOBAL)

set(
    LIBCZI_JULIA_ZSTD_ROOT
    ""
    CACHE PATH
    "Prefix containing the externally supplied zstd headers and library"
)

if(LIBCZI_JULIA_ZSTD_ROOT)
    find_path(
        zstd_INCLUDE_DIR
        NAMES zstd.h
        PATHS "${LIBCZI_JULIA_ZSTD_ROOT}/include"
        NO_DEFAULT_PATH
        REQUIRED
    )
    find_library(
        zstd_LIBRARY
        NAMES zstd libzstd
        PATHS
            "${LIBCZI_JULIA_ZSTD_ROOT}/lib"
            "${LIBCZI_JULIA_ZSTD_ROOT}/bin"
        NO_DEFAULT_PATH
        REQUIRED
    )
else()
    find_path(zstd_INCLUDE_DIR NAMES zstd.h REQUIRED)
    find_library(zstd_LIBRARY NAMES zstd libzstd REQUIRED)
endif()

if(NOT TARGET zstd::libzstd_shared)
    add_library(zstd::libzstd_shared INTERFACE IMPORTED)
    set_target_properties(
        zstd::libzstd_shared
        PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${zstd_INCLUDE_DIR}"
            INTERFACE_LINK_LIBRARIES "${zstd_LIBRARY}"
    )
endif()

set(zstd_FOUND TRUE)
