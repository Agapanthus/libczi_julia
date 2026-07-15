# BinaryBuilder platforms

The recipe builds every current 64-bit little-endian BinaryBuilder target:

- glibc Linux: x86-64, AArch64, PowerPC64LE, RISC-V 64;
- musl Linux: x86-64 and AArch64;
- macOS: x86-64 and AArch64;
- FreeBSD: x86-64 and AArch64;
- Windows: x86-64.

The filter is architecture-based:

```julia
const SUPPORTED_ARCHITECTURES = (
    "x86_64",
    "aarch64",
    "powerpc64le",
    "riscv64",
)

platforms = filter(
    platform -> arch(platform) in SUPPORTED_ARCHITECTURES,
    supported_platforms(),
)
```

Thirty-two-bit targets remain excluded until libCZI and the public C ABI have
been tested on them.

## macOS SDK acceptance

Use either:

```sh
BINARYBUILDER_AUTOMATIC_APPLE=true \
julia --project=yggdrasil yggdrasil/build_tarballs.jl
```

or:

```sh
export BINARYBUILDER_AUTOMATIC_APPLE=true
julia --project=yggdrasil yggdrasil/build_tarballs.jl
```

This does not export the variable:

```sh
BINARYBUILDER_AUTOMATIC_APPLE=true;
julia ...
```

The semicolon completes a shell-only assignment; without `export`, the Julia
child process does not receive it.
