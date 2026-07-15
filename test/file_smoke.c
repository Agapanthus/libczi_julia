#include "libczi_julia.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

static int check(lcj_status status, const char* operation)
{
    if (status == LCJ_OK) {
        return 1;
    }

    fprintf(
        stderr,
        "%s failed (%d): %s\n",
        operation,
        (int)status,
        lcj_last_error_message());
    return 0;
}

int main(int argc, char** argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: %s FILE.czi\n", argv[0]);
        return 2;
    }

    int result = 1;
    lcj_reader* reader = NULL;
    lcj_bitmap* bitmap = NULL;
    void* pixels = NULL;

    if (!check(
            lcj_reader_open_utf8(argv[1], &reader),
            "lcj_reader_open_utf8")) {
        goto cleanup;
    }

    lcj_statistics statistics;
    if (!check(
            lcj_reader_statistics(reader, &statistics),
            "lcj_reader_statistics")) {
        goto cleanup;
    }
    if (statistics.subblock_count <= 0) {
        fprintf(stderr, "fixture contains no subblocks\n");
        goto cleanup;
    }

    size_t metadata_size = 0;
    if (!check(
            lcj_reader_metadata_size(reader, &metadata_size),
            "lcj_reader_metadata_size")) {
        goto cleanup;
    }
    if (metadata_size == 0) {
        fprintf(stderr, "fixture contains no metadata XML\n");
        goto cleanup;
    }

    lcj_subblock_info subblock;
    if (!check(
            lcj_reader_subblock_info(reader, 0, &subblock),
            "lcj_reader_subblock_info")) {
        goto cleanup;
    }

    if (!check(
            lcj_reader_read_subblock_bitmap(reader, 0, &bitmap),
            "lcj_reader_read_subblock_bitmap")) {
        goto cleanup;
    }

    lcj_bitmap_info bitmap_info;
    if (!check(
            lcj_bitmap_get_info(bitmap, &bitmap_info),
            "lcj_bitmap_get_info")) {
        goto cleanup;
    }

    if (bitmap_info.height != 0 &&
        bitmap_info.row_bytes > SIZE_MAX / bitmap_info.height) {
        fprintf(stderr, "decoded bitmap size overflows size_t\n");
        goto cleanup;
    }

    const size_t pixel_bytes =
        (size_t)bitmap_info.row_bytes * bitmap_info.height;
    pixels = malloc(pixel_bytes);
    if (pixels == NULL) {
        fprintf(stderr, "failed to allocate %zu decoded bytes\n", pixel_bytes);
        goto cleanup;
    }

    if (!check(
            lcj_bitmap_copy(
                bitmap,
                pixels,
                pixel_bytes,
                (size_t)bitmap_info.row_bytes),
            "lcj_bitmap_copy")) {
        goto cleanup;
    }

    printf(
        "subblocks=%" PRId32
        " first=%" PRIu32 "x%" PRIu32
        " metadata=%zu bytes\n",
        statistics.subblock_count,
        bitmap_info.width,
        bitmap_info.height,
        metadata_size);
    result = 0;

cleanup:
    free(pixels);
    lcj_bitmap_close(bitmap);
    lcj_reader_close(reader);
    return result;
}
