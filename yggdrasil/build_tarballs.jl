using BinaryBuilder, Pkg

name = "libczi_julia"
version = v"0.2.1"

const LIBCZI_JULIA_REPOSITORY =
    "https://github.com/Agapanthus/libczi_julia.git"
const LIBCZI_REPOSITORY =
    "https://github.com/ZEISS/libczi.git"
const LIBCZI_COMMIT =
    "61f74ff097d6d0fbe6e36f204ff59d92e299d7cd"

wrapper_commit = get(ENV, "LIBCZI_JULIA_COMMIT", "")
occursin(r"^[0-9a-f]{40}$", wrapper_commit) || error(
    "set LIBCZI_JULIA_COMMIT to the 40-character commit containing " *
    "the hand-written C ABI wrapper",
)

sources = [
    GitSource(LIBCZI_JULIA_REPOSITORY, wrapper_commit),
    GitSource(LIBCZI_REPOSITORY, LIBCZI_COMMIT),
]

script = raw"""
cd ${WORKSPACE}/srcdir/libczi_julia

cmake -S . -B build -G "Unix Makefiles" \
    -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TARGET_TOOLCHAIN} \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=${prefix} \
    -DLIBCZI_SOURCE_DIR=${WORKSPACE}/srcdir/libczi \
    -DLIBCZI_BUILD_PREFER_EXTERNALPACKAGE_EIGEN3=ON \
    -DLIBCZI_BUILD_PREFER_EXTERNALPACKAGE_ZSTD=ON \
    -Dzstd_DIR=${WORKSPACE}/srcdir/libczi_julia/cmake/zstd \
    -DLIBCZI_JULIA_ZSTD_ROOT=${prefix} \
    -DCRASH_ON_UNALIGNED_ACCESS=0 \
    -DADDITIONAL_LIBS_REQUIRED_FOR_ATOMIC="" \
    -DBUILD_TESTING=OFF

cmake --build build --parallel ${nproc}
cmake --install build

install_license \
    ${WORKSPACE}/srcdir/libczi_julia/COPYING \
    ${WORKSPACE}/srcdir/libczi/COPYING \
    ${WORKSPACE}/srcdir/libczi/COPYING.LESSER
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
    Dependency("Zstd_jll"),
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
