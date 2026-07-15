using BinaryBuilder, Pkg

name = "libczi_julia"
version = v"0.2.2"

const LIBCZI_JULIA_COMMIT =
    "0de3b4407bf13508e56f2b1d678616558dc7d737"
const LIBCZI_COMMIT =
    "61f74ff097d6d0fbe6e36f204ff59d92e299d7cd"
const ZSTD_VERSION = v"1.5.7"

sources = [
    GitSource(
        "https://github.com/Agapanthus/libczi_julia.git",
        LIBCZI_JULIA_COMMIT,
    ),
    GitSource(
        "https://github.com/ZEISS/libczi.git",
        LIBCZI_COMMIT,
    ),
    ArchiveSource(
        "https://github.com/facebook/zstd/releases/download/" *
        "v$(ZSTD_VERSION)/zstd-$(ZSTD_VERSION).tar.gz",
        "eb33e51f49a15e023950cd7825ca74a4a2b43db8354825ac24fc1b7ee09e6fa3",
    ),
]

script = raw"""
cd ${WORKSPACE}/srcdir/libczi_julia

cmake -S . -B build -G "Unix Makefiles" \
    -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TARGET_TOOLCHAIN} \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=${prefix} \
    -DLIBCZI_SOURCE_DIR=${WORKSPACE}/srcdir/libczi \
    -DLIBCZI_BUILD_PREFER_EXTERNALPACKAGE_EIGEN3=ON \
    -DLIBCZI_BUILD_PREFER_EXTERNALPACKAGE_ZSTD=OFF \
    -DFETCHCONTENT_SOURCE_DIR_ZSTD=${WORKSPACE}/srcdir/zstd-1.5.7 \
    -DFETCHCONTENT_FULLY_DISCONNECTED=ON \
    -DCRASH_ON_UNALIGNED_ACCESS=0 \
    -DADDITIONAL_LIBS_REQUIRED_FOR_ATOMIC="" \
    -DBUILD_TESTING=OFF

cmake --build build --parallel ${nproc}
cmake --install build

install_license \
    ${WORKSPACE}/srcdir/libczi_julia/COPYING \
    ${WORKSPACE}/srcdir/libczi/COPYING \
    ${WORKSPACE}/srcdir/libczi/COPYING.LESSER     ${WORKSPACE}/srcdir/zstd-1.5.7/LICENSE
"""

platforms = filter(supported_platforms()) do platform
    (
        os(platform) == "linux" &&
        arch(platform) in ("x86_64", "aarch64") &&
        libc(platform) == "glibc"
    ) || (
        os(platform) == "macos" &&
        arch(platform) in ("x86_64", "aarch64")
    ) || (
        os(platform) == "windows" &&
        arch(platform) == "x86_64"
    )
end

products = [
    LibraryProduct(["libczi_julia", "czi_julia"], :libczi_julia),
]

dependencies = [
    HostBuildDependency("CMake_jll"),
    BuildDependency("Eigen_jll"),
]

build_tarballs(
    ARGS,
    name,
    version,
    sources,
    script,
    platforms,
    products,
    dependencies;
    julia_compat="1.10",
)
