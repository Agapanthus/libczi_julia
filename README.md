# libczi_julia

Small hand-written C ABI around ZEISS libCZI.

The public boundary contains only opaque handles, fixed-width C structures,
caller-owned buffers, status codes, and an ABI version. Julia never depends on
libCZI C++ layouts.

## Native build

```sh
cmake -S . -B build -G "Unix Makefiles" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$PWD/.native" \
    -DLIBCZI_SOURCE_DIR=/absolute/path/to/libczi \
    -DLIBCZI_JULIA_TEST_FILE=/absolute/path/to/test.czi \
    -DLIBCZI_BUILD_PREFER_EXTERNALPACKAGE_EIGEN3=OFF \
    -DLIBCZI_BUILD_PREFER_EXTERNALPACKAGE_ZSTD=OFF

cmake --build build --parallel
ctest --test-dir build --output-on-failure
cmake --install build
```

Local builds may let libCZI fetch Eigen and zstd. The BinaryBuilder recipe is
offline: it supplies Eigen through `Eigen_jll` and supplies the pinned zstd
source through `FETCHCONTENT_SOURCE_DIR_ZSTD`.

## BinaryBuilder

There is one recipe:

```sh
julia --project=yggdrasil \
    yggdrasil/build_tarballs.jl \
    --verbose x86_64-linux-gnu
```

The recipe hardcodes:

- wrapper commit `0de3b4407bf13508e56f2b1d678616558dc7d737`;
- libCZI commit `61f74ff097d6d0fbe6e36f204ff59d92e299d7cd`;
- zstd 1.5.7 archive and SHA-256.

After committing this revision, replace the wrapper commit constant with the
new commit before publishing the recipe.

## ABI policy

- ABI major 1 is stable for compatible additions.
- Every native allocation has an explicit close function.
- Returned metadata and bitmap data are copied into caller-owned storage.
- No C++ object, exception, callback, or standard-library type crosses the C
  boundary.

## BinaryBuilder compiler selection

The wrapper requires C++17. The recipe requests GCC 12 and LLVM 13 explicitly;
BinaryBuilder otherwise defaults to GCC 4.8.5 for compatibility, which does
not expose CMake's `cxx_std_17` compile feature.
