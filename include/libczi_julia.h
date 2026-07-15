#ifndef LIBCZI_JULIA_H
#define LIBCZI_JULIA_H

#include <stddef.h>
#include <stdint.h>

#if defined(_WIN32)
#  if defined(LIBCZI_JULIA_BUILDING)
#    define LCJ_API __declspec(dllexport)
#  else
#    define LCJ_API __declspec(dllimport)
#  endif
#else
#  define LCJ_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define LCJ_ABI_VERSION_MAJOR 1u
#define LCJ_ABI_VERSION_MINOR 0u
#define LCJ_DIMENSION_COUNT 9u

typedef struct lcj_reader lcj_reader;
typedef struct lcj_bitmap lcj_bitmap;

typedef enum lcj_status {
    LCJ_OK = 0,
    LCJ_INVALID_ARGUMENT = 1,
    LCJ_OUT_OF_RANGE = 2,
    LCJ_IO_ERROR = 3,
    LCJ_UNSUPPORTED = 4,
    LCJ_BUFFER_TOO_SMALL = 5,
    LCJ_OUT_OF_MEMORY = 6,
    LCJ_INTERNAL_ERROR = 7
} lcj_status;

typedef enum lcj_dimension {
    LCJ_DIM_Z = 0,
    LCJ_DIM_C = 1,
    LCJ_DIM_T = 2,
    LCJ_DIM_R = 3,
    LCJ_DIM_S = 4,
    LCJ_DIM_I = 5,
    LCJ_DIM_H = 6,
    LCJ_DIM_V = 7,
    LCJ_DIM_B = 8
} lcj_dimension;

typedef enum lcj_pixel_type {
    LCJ_PIXEL_GRAY8 = 0,
    LCJ_PIXEL_GRAY16 = 1,
    LCJ_PIXEL_GRAY32_FLOAT = 2,
    LCJ_PIXEL_BGR24 = 3,
    LCJ_PIXEL_BGR48 = 4,
    LCJ_PIXEL_BGR96_FLOAT = 8,
    LCJ_PIXEL_INVALID = 255
} lcj_pixel_type;

typedef struct lcj_version {
    uint32_t major;
    uint32_t minor;
    uint32_t patch;
    uint32_t tweak;
} lcj_version;

typedef struct lcj_rect_i32 {
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
} lcj_rect_i32;

typedef struct lcj_dim_bounds {
    uint8_t present;
    uint8_t reserved[3];
    int32_t start;
    int32_t size;
} lcj_dim_bounds;

typedef struct lcj_statistics {
    int32_t subblock_count;
    int32_t min_m_index;
    int32_t max_m_index;
    uint8_t m_index_present;
    uint8_t reserved[3];
    lcj_rect_i32 bounding_box;
    lcj_rect_i32 bounding_box_layer0;
} lcj_statistics;

typedef struct lcj_subblock_info {
    int32_t native_index;
    int32_t compression_raw;
    uint8_t pixel_type;
    uint8_t pyramid_type;
    uint8_t m_index_present;
    uint8_t reserved0;
    int32_t m_index;
    lcj_rect_i32 logical_rect;
    uint32_t physical_width;
    uint32_t physical_height;
    uint16_t coordinate_mask;
    uint16_t reserved1;
    int32_t coordinate[LCJ_DIMENSION_COUNT];
} lcj_subblock_info;

typedef struct lcj_bitmap_info {
    uint8_t pixel_type;
    uint8_t reserved[3];
    uint32_t width;
    uint32_t height;
    uint32_t reserved1;
    uint64_t row_bytes;
} lcj_bitmap_info;

/*
 * The returned pointer remains valid until the next libczi_julia call on the
 * same thread. It must not be freed.
 */
LCJ_API const char* lcj_last_error_message(void);

LCJ_API lcj_status lcj_abi_version(lcj_version* version);
LCJ_API lcj_status lcj_libczi_version(lcj_version* version);

LCJ_API lcj_status lcj_reader_open_utf8(
    const char* path,
    lcj_reader** reader);

LCJ_API lcj_status lcj_reader_close(lcj_reader* reader);

LCJ_API lcj_status lcj_reader_statistics(
    lcj_reader* reader,
    lcj_statistics* statistics);

LCJ_API lcj_status lcj_reader_dimension_bounds(
    lcj_reader* reader,
    lcj_dimension dimension,
    lcj_dim_bounds* bounds);

LCJ_API lcj_status lcj_reader_metadata_size(
    lcj_reader* reader,
    size_t* size);

LCJ_API lcj_status lcj_reader_metadata_copy(
    lcj_reader* reader,
    void* destination,
    size_t destination_size);

LCJ_API lcj_status lcj_reader_subblock_info(
    lcj_reader* reader,
    int32_t native_index,
    lcj_subblock_info* info);

LCJ_API lcj_status lcj_reader_read_subblock_bitmap(
    lcj_reader* reader,
    int32_t native_index,
    lcj_bitmap** bitmap);

LCJ_API lcj_status lcj_bitmap_get_info(
    lcj_bitmap* bitmap,
    lcj_bitmap_info* info);

/*
 * Copy the decoded bitmap into caller-owned storage.
 *
 * `destination_row_stride` is measured in bytes and must be at least
 * `lcj_bitmap_info.row_bytes`. `destination_size` must cover the last copied
 * row. The wrapper performs no pixel conversion.
 */
LCJ_API lcj_status lcj_bitmap_copy(
    lcj_bitmap* bitmap,
    void* destination,
    size_t destination_size,
    size_t destination_row_stride);

LCJ_API lcj_status lcj_bitmap_close(lcj_bitmap* bitmap);

#ifdef __cplusplus
}
#endif

#endif
