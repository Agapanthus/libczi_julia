# Yggdrasil recipe

`build_tarballs.jl` is the only BinaryBuilder recipe.

It uses immutable sources for the wrapper, libCZI, and zstd, and builds with
Unix Makefiles. zstd is compiled through libCZI's existing FetchContent path;
there is no custom `zstdConfig.cmake` and no `Zstd_jll` dependency.

Before submission, update `LIBCZI_JULIA_COMMIT` in `build_tarballs.jl` to the
commit containing this source revision.

```sh
julia --project=yggdrasil \
    yggdrasil/build_tarballs.jl \
    --verbose x86_64-linux-gnu
```

The recipe targets every current 64-bit little-endian BinaryBuilder platform.
See `../PLATFORMS.md`.

The recipe pins `preferred_gcc_version=v"8"` and
`preferred_llvm_version=v"13"` because the wrapper requires C++17.
