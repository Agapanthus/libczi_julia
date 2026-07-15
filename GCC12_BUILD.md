# GCC 12 BinaryBuilder build

The pinned libCZI source is not modified.

The single BinaryBuilder recipe selects GCC 12:

```julia
preferred_gcc_version=v"12"
```

The wrapper remains C++17 and libCZI retains its upstream C++ standard.

The repository has two commits:

1. the complete source revision;
2. the recipe-only commit pinning the first commit as `LIBCZI_JULIA_COMMIT`.

This avoids a self-referential Git source hash.
