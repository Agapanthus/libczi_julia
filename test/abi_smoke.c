#include "libczi_julia.h"

#include <stdio.h>
#include <stddef.h>

_Static_assert(sizeof(lcj_version) == 16, "lcj_version ABI size");
_Static_assert(sizeof(lcj_rect_i32) == 16, "lcj_rect_i32 ABI size");
_Static_assert(sizeof(lcj_dim_bounds) == 12, "lcj_dim_bounds ABI size");
_Static_assert(sizeof(lcj_statistics) == 48, "lcj_statistics ABI size");
_Static_assert(sizeof(lcj_subblock_info) == 80, "lcj_subblock_info ABI size");
_Static_assert(sizeof(lcj_bitmap_info) == 24, "lcj_bitmap_info ABI size");
_Static_assert(
    offsetof(lcj_subblock_info, coordinate) == 44,
    "lcj_subblock_info coordinate offset");
_Static_assert(
    offsetof(lcj_bitmap_info, row_bytes) == 16,
    "lcj_bitmap_info row_bytes offset");

int main(void)
{
    lcj_version abi = {0, 0, 0, 0};
    lcj_version libczi = {0, 0, 0, 0};

    if (lcj_abi_version(&abi) != LCJ_OK) {
        fprintf(stderr, "%s\n", lcj_last_error_message());
        return 1;
    }
    if (abi.major != LCJ_ABI_VERSION_MAJOR) {
        fprintf(stderr, "unexpected ABI major: %u\n", abi.major);
        return 2;
    }
    if (lcj_libczi_version(&libczi) != LCJ_OK) {
        fprintf(stderr, "%s\n", lcj_last_error_message());
        return 3;
    }

    printf(
        "libczi_julia ABI %u.%u; libCZI %u.%u.%u.%u\n",
        abi.major,
        abi.minor,
        libczi.major,
        libczi.minor,
        libczi.patch,
        libczi.tweak);
    return 0;
}
