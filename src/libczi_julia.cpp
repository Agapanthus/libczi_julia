#include "libczi_julia.h"

#include "libCZI.h"
#include "libCZI_exceptions.h"

#include <algorithm>
#include <codecvt>
#include <cstddef>
#include <cstring>
#include <exception>
#include <ios>
#include <iterator>
#include <limits>
#include <locale>
#include <memory>
#include <new>
#include <stdexcept>
#include <string>

struct lcj_reader {
    std::shared_ptr<libCZI::ICZIReader> value;
};

struct lcj_bitmap {
    std::shared_ptr<libCZI::IBitmapData> value;
};

static_assert(sizeof(lcj_version) == 16, "lcj_version ABI size changed");
static_assert(sizeof(lcj_rect_i32) == 16, "lcj_rect_i32 ABI size changed");
static_assert(sizeof(lcj_dim_bounds) == 12, "lcj_dim_bounds ABI size changed");
static_assert(sizeof(lcj_statistics) == 48, "lcj_statistics ABI size changed");
static_assert(sizeof(lcj_subblock_info) == 80, "lcj_subblock_info ABI size changed");
static_assert(sizeof(lcj_bitmap_info) == 24, "lcj_bitmap_info ABI size changed");

static_assert(
    offsetof(lcj_subblock_info, coordinate) == 44,
    "lcj_subblock_info coordinate offset changed");
static_assert(
    offsetof(lcj_bitmap_info, row_bytes) == 16,
    "lcj_bitmap_info row_bytes offset changed");

namespace {

thread_local std::string last_error;

class buffer_too_small : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class unsupported_operation : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

void clear_error()
{
    last_error.clear();
}

lcj_status fail(lcj_status status, const char* message)
{
    last_error = message == nullptr ? "" : message;
    return status;
}

template<class Function>
lcj_status protect(Function&& function)
{
    clear_error();

    try {
        function();
        return LCJ_OK;
    }
    catch (const std::bad_alloc& error) {
        return fail(LCJ_OUT_OF_MEMORY, error.what());
    }
    catch (const libCZI::LibCZIIOException& error) {
        return fail(LCJ_IO_ERROR, error.what());
    }
    catch (const libCZI::LibCZISegmentNotPresent& error) {
        return fail(LCJ_UNSUPPORTED, error.what());
    }
    catch (const std::invalid_argument& error) {
        return fail(LCJ_INVALID_ARGUMENT, error.what());
    }
    catch (const buffer_too_small& error) {
        return fail(LCJ_BUFFER_TOO_SMALL, error.what());
    }
    catch (const unsupported_operation& error) {
        return fail(LCJ_UNSUPPORTED, error.what());
    }
    catch (const std::out_of_range& error) {
        return fail(LCJ_OUT_OF_RANGE, error.what());
    }
    catch (const std::ios_base::failure& error) {
        return fail(LCJ_IO_ERROR, error.what());
    }
    catch (const std::exception& error) {
        return fail(LCJ_INTERNAL_ERROR, error.what());
    }
    catch (...) {
        return fail(LCJ_INTERNAL_ERROR, "unknown native exception");
    }
}

std::wstring utf8_to_wstring(const char* path)
{
#if defined(_WIN32)
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conversion;
#else
    std::wstring_convert<std::codecvt_utf8<wchar_t>> conversion;
#endif
    return conversion.from_bytes(path);
}

libCZI::DimensionIndex to_libczi_dimension(lcj_dimension dimension)
{
    switch (dimension) {
    case LCJ_DIM_Z: return libCZI::DimensionIndex::Z;
    case LCJ_DIM_C: return libCZI::DimensionIndex::C;
    case LCJ_DIM_T: return libCZI::DimensionIndex::T;
    case LCJ_DIM_R: return libCZI::DimensionIndex::R;
    case LCJ_DIM_S: return libCZI::DimensionIndex::S;
    case LCJ_DIM_I: return libCZI::DimensionIndex::I;
    case LCJ_DIM_H: return libCZI::DimensionIndex::H;
    case LCJ_DIM_V: return libCZI::DimensionIndex::V;
    case LCJ_DIM_B: return libCZI::DimensionIndex::B;
    }

    throw std::invalid_argument("invalid CZI dimension");
}


uint8_t to_lcj_pixel_type(libCZI::PixelType pixel_type)
{
    switch (pixel_type) {
    case libCZI::PixelType::Gray8: return LCJ_PIXEL_GRAY8;
    case libCZI::PixelType::Gray16: return LCJ_PIXEL_GRAY16;
    case libCZI::PixelType::Gray32Float: return LCJ_PIXEL_GRAY32_FLOAT;
    case libCZI::PixelType::Bgr24: return LCJ_PIXEL_BGR24;
    case libCZI::PixelType::Bgr48: return LCJ_PIXEL_BGR48;
    case libCZI::PixelType::Bgr96Float: return LCJ_PIXEL_BGR96_FLOAT;
    default: return LCJ_PIXEL_INVALID;
    }
}

uint8_t to_lcj_pyramid_type(libCZI::SubBlockPyramidType pyramid_type)
{
    switch (pyramid_type) {
    case libCZI::SubBlockPyramidType::None: return 0;
    case libCZI::SubBlockPyramidType::SingleSubBlock: return 1;
    case libCZI::SubBlockPyramidType::MultiSubBlock: return 2;
    default: return 255;
    }
}

const libCZI::DimensionIndex dimensions[LCJ_DIMENSION_COUNT] = {
    libCZI::DimensionIndex::Z,
    libCZI::DimensionIndex::C,
    libCZI::DimensionIndex::T,
    libCZI::DimensionIndex::R,
    libCZI::DimensionIndex::S,
    libCZI::DimensionIndex::I,
    libCZI::DimensionIndex::H,
    libCZI::DimensionIndex::V,
    libCZI::DimensionIndex::B,
};

lcj_rect_i32 convert_rect(const libCZI::IntRect& rectangle)
{
    return {
        static_cast<int32_t>(rectangle.x),
        static_cast<int32_t>(rectangle.y),
        static_cast<int32_t>(rectangle.w),
        static_cast<int32_t>(rectangle.h),
    };
}

size_t bytes_per_pixel(libCZI::PixelType pixel_type)
{
    switch (pixel_type) {
    case libCZI::PixelType::Gray8: return 1;
    case libCZI::PixelType::Gray16: return 2;
    case libCZI::PixelType::Gray32Float: return 4;
    case libCZI::PixelType::Bgr24: return 3;
    case libCZI::PixelType::Bgr48: return 6;
    case libCZI::PixelType::Bgr96Float: return 12;
    default:
        throw unsupported_operation("unsupported decoded pixel type");
    }
}

void require_reader(lcj_reader* reader)
{
    if (reader == nullptr || !reader->value) {
        throw std::invalid_argument("reader must not be null");
    }
}

void require_bitmap(lcj_bitmap* bitmap)
{
    if (bitmap == nullptr || !bitmap->value) {
        throw std::invalid_argument("bitmap must not be null");
    }
}

std::shared_ptr<libCZI::IMetadataSegment> metadata_segment(lcj_reader* reader)
{
    require_reader(reader);
    auto segment = reader->value->ReadMetadataSegment();
    if (!segment) {
        throw unsupported_operation("CZI metadata segment is absent");
    }
    return segment;
}


} // namespace

extern "C" {

const char* lcj_last_error_message(void)
{
    return last_error.c_str();
}

lcj_status lcj_abi_version(lcj_version* version)
{
    if (version == nullptr) {
        return fail(LCJ_INVALID_ARGUMENT, "version must not be null");
    }

    clear_error();
    *version = {
        LCJ_ABI_VERSION_MAJOR,
        LCJ_ABI_VERSION_MINOR,
        0u,
        0u,
    };
    return LCJ_OK;
}

lcj_status lcj_libczi_version(lcj_version* version)
{
    if (version == nullptr) {
        return fail(LCJ_INVALID_ARGUMENT, "version must not be null");
    }

    return protect([&] {
        int major = 0;
        int minor = 0;
        int patch = 0;
        int tweak = 0;
        libCZI::GetLibCZIVersion(&major, &minor, &patch, &tweak);
        *version = {
            static_cast<uint32_t>(major),
            static_cast<uint32_t>(minor),
            static_cast<uint32_t>(patch),
            static_cast<uint32_t>(tweak),
        };
    });
}

lcj_status lcj_reader_open_utf8(
    const char* path,
    lcj_reader** reader)
{
    if (path == nullptr || reader == nullptr) {
        return fail(
            LCJ_INVALID_ARGUMENT,
            "path and reader output must not be null");
    }

    *reader = nullptr;

    return protect([&] {
        const auto wide_path = utf8_to_wstring(path);
        auto stream = libCZI::CreateStreamFromFile(wide_path.c_str());
        auto native_reader = libCZI::CreateCZIReader();
        native_reader->Open(stream);

        auto result = std::make_unique<lcj_reader>();
        result->value = std::move(native_reader);
        *reader = result.release();
    });
}

lcj_status lcj_reader_close(lcj_reader* reader)
{
    if (reader == nullptr) {
        clear_error();
        return LCJ_OK;
    }

    std::unique_ptr<lcj_reader> owned(reader);
    return protect([&] {
        if (owned->value) {
            owned->value->Close();
            owned->value.reset();
        }
    });
}

lcj_status lcj_reader_statistics(
    lcj_reader* reader,
    lcj_statistics* statistics)
{
    if (statistics == nullptr) {
        return fail(LCJ_INVALID_ARGUMENT, "statistics must not be null");
    }

    return protect([&] {
        require_reader(reader);
        const auto value = reader->value->GetStatistics();

        statistics->subblock_count =
            static_cast<int32_t>(value.subBlockCount);
        statistics->min_m_index =
            static_cast<int32_t>(value.minMindex);
        statistics->max_m_index =
            static_cast<int32_t>(value.maxMindex);
        statistics->m_index_present =
            value.IsMIndexValid() ? uint8_t{1} : uint8_t{0};
        std::fill(
            std::begin(statistics->reserved),
            std::end(statistics->reserved),
            uint8_t{0});
        statistics->bounding_box =
            convert_rect(value.boundingBox);
        statistics->bounding_box_layer0 =
            convert_rect(value.boundingBoxLayer0Only);
    });
}

lcj_status lcj_reader_dimension_bounds(
    lcj_reader* reader,
    lcj_dimension dimension,
    lcj_dim_bounds* bounds)
{
    if (bounds == nullptr) {
        return fail(LCJ_INVALID_ARGUMENT, "bounds must not be null");
    }

    return protect([&] {
        require_reader(reader);
        const auto statistics = reader->value->GetStatistics();

        int start = 0;
        int size = 0;
        const bool present = statistics.dimBounds.TryGetInterval(
            to_libczi_dimension(dimension),
            &start,
            &size);

        bounds->present = present ? uint8_t{1} : uint8_t{0};
        std::fill(
            std::begin(bounds->reserved),
            std::end(bounds->reserved),
            uint8_t{0});
        bounds->start = present ? static_cast<int32_t>(start) : 0;
        bounds->size = present ? static_cast<int32_t>(size) : 0;
    });
}

lcj_status lcj_reader_metadata_size(
    lcj_reader* reader,
    size_t* size)
{
    if (size == nullptr) {
        return fail(LCJ_INVALID_ARGUMENT, "size must not be null");
    }

    return protect([&] {
        auto segment = metadata_segment(reader);
        const void* data = nullptr;
        size_t bytes = 0;
        segment->DangerousGetRawData(
            libCZI::IMetadataSegment::MemBlkType::XmlMetadata,
            data,
            bytes);
        *size = bytes;
    });
}

lcj_status lcj_reader_metadata_copy(
    lcj_reader* reader,
    void* destination,
    size_t destination_size)
{
    return protect([&] {
        auto segment = metadata_segment(reader);
        const void* data = nullptr;
        size_t bytes = 0;
        segment->DangerousGetRawData(
            libCZI::IMetadataSegment::MemBlkType::XmlMetadata,
            data,
            bytes);

        if (bytes > destination_size) {
            throw buffer_too_small("metadata destination is too small");
        }
        if (bytes != 0 && destination == nullptr) {
            throw std::invalid_argument(
                "metadata destination must not be null");
        }

        if (bytes != 0 && data == nullptr) {
            throw std::runtime_error(
                "libCZI returned null metadata storage");
        }
        if (bytes != 0) {
            std::memcpy(destination, data, bytes);
        }
    });
}

lcj_status lcj_reader_subblock_info(
    lcj_reader* reader,
    int32_t native_index,
    lcj_subblock_info* info)
{
    if (info == nullptr) {
        return fail(LCJ_INVALID_ARGUMENT, "subblock info must not be null");
    }

    return protect([&] {
        require_reader(reader);

        libCZI::SubBlockInfo native_info;
        if (!reader->value->TryGetSubBlockInfo(native_index, &native_info)) {
            throw std::out_of_range("subblock index is out of range");
        }

        info->native_index = native_index;
        info->compression_raw =
            static_cast<int32_t>(native_info.compressionModeRaw);
        info->pixel_type =
            to_lcj_pixel_type(native_info.pixelType);
        info->pyramid_type =
            to_lcj_pyramid_type(native_info.pyramidType);
        info->m_index_present =
            native_info.IsMindexValid() ? uint8_t{1} : uint8_t{0};
        info->reserved0 = 0;
        info->m_index = native_info.IsMindexValid()
            ? static_cast<int32_t>(native_info.mIndex)
            : 0;
        info->logical_rect = convert_rect(native_info.logicalRect);
        if (static_cast<uint64_t>(native_info.physicalSize.w) >
                std::numeric_limits<uint32_t>::max() ||
            static_cast<uint64_t>(native_info.physicalSize.h) >
                std::numeric_limits<uint32_t>::max()) {
            throw std::runtime_error(
                "libCZI returned an invalid physical subblock size");
        }
        info->physical_width =
            static_cast<uint32_t>(native_info.physicalSize.w);
        info->physical_height =
            static_cast<uint32_t>(native_info.physicalSize.h);
        info->coordinate_mask = 0;
        info->reserved1 = 0;
        std::fill(
            std::begin(info->coordinate),
            std::end(info->coordinate),
            int32_t{0});

        for (size_t i = 0; i < LCJ_DIMENSION_COUNT; ++i) {
            int coordinate = 0;
            if (native_info.coordinate.TryGetPosition(
                    dimensions[i],
                    &coordinate)) {
                info->coordinate_mask |=
                    static_cast<uint16_t>(uint16_t{1} << i);
                info->coordinate[i] =
                    static_cast<int32_t>(coordinate);
            }
        }
    });
}

lcj_status lcj_reader_read_subblock_bitmap(
    lcj_reader* reader,
    int32_t native_index,
    lcj_bitmap** bitmap)
{
    if (bitmap == nullptr) {
        return fail(LCJ_INVALID_ARGUMENT, "bitmap output must not be null");
    }

    *bitmap = nullptr;

    return protect([&] {
        require_reader(reader);

        auto subblock = reader->value->ReadSubBlock(native_index);
        if (!subblock) {
            throw std::out_of_range("subblock index is out of range");
        }

        auto decoded = subblock->CreateBitmap();
        if (!decoded) {
            throw std::runtime_error("libCZI returned a null bitmap");
        }

        auto result = std::make_unique<lcj_bitmap>();
        result->value = std::move(decoded);
        *bitmap = result.release();
    });
}

lcj_status lcj_bitmap_get_info(
    lcj_bitmap* bitmap,
    lcj_bitmap_info* info)
{
    if (info == nullptr) {
        return fail(LCJ_INVALID_ARGUMENT, "bitmap info must not be null");
    }

    return protect([&] {
        require_bitmap(bitmap);

        const auto pixel_type = bitmap->value->GetPixelType();
        const auto size = bitmap->value->GetSize();
        const auto bytes = bytes_per_pixel(pixel_type);

        if (size.w > std::numeric_limits<size_t>::max() / bytes) {
            throw std::overflow_error("bitmap row size overflows size_t");
        }

        info->pixel_type = to_lcj_pixel_type(pixel_type);
        std::fill(
            std::begin(info->reserved),
            std::end(info->reserved),
            uint8_t{0});
        info->width = size.w;
        info->height = size.h;
        info->reserved1 = 0;
        info->row_bytes =
            static_cast<uint64_t>(static_cast<size_t>(size.w) * bytes);
    });
}

lcj_status lcj_bitmap_copy(
    lcj_bitmap* bitmap,
    void* destination,
    size_t destination_size,
    size_t destination_row_stride)
{
    return protect([&] {
        require_bitmap(bitmap);
        if (destination == nullptr) {
            throw std::invalid_argument(
                "bitmap destination must not be null");
        }

        const auto pixel_type = bitmap->value->GetPixelType();
        const auto size = bitmap->value->GetSize();
        const auto bytes = bytes_per_pixel(pixel_type);
        if (size.w > std::numeric_limits<size_t>::max() / bytes) {
            throw std::overflow_error("bitmap row size overflows size_t");
        }
        const auto row_bytes =
            static_cast<size_t>(size.w) * bytes;

        if (destination_row_stride < row_bytes) {
            throw buffer_too_small(
                "bitmap destination row stride is too small");
        }

        size_t required = 0;
        if (size.h != 0) {
            const auto rows_before_last =
                static_cast<size_t>(size.h - 1);
            if (rows_before_last >
                (std::numeric_limits<size_t>::max() - row_bytes) /
                    destination_row_stride) {
                throw std::overflow_error(
                    "bitmap destination size overflows size_t");
            }
            required =
                rows_before_last * destination_row_stride + row_bytes;
        }

        if (destination_size < required) {
            throw buffer_too_small(
                "bitmap destination buffer is too small");
        }

        libCZI::ScopedBitmapLockerSP lock(bitmap->value);
        const auto* source =
            static_cast<const uint8_t*>(lock.ptrDataRoi);
        auto* target = static_cast<uint8_t*>(destination);

        if (size.w != 0 && size.h != 0 && source == nullptr) {
            throw std::runtime_error(
                "libCZI returned null decoded bitmap storage");
        }
        if (lock.stride < row_bytes) {
            throw std::runtime_error(
                "libCZI returned a bitmap stride smaller than one row");
        }

        for (uint32_t y = 0; y < size.h; ++y) {
            std::memcpy(
                target + static_cast<size_t>(y) *
                    destination_row_stride,
                source + static_cast<size_t>(y) * lock.stride,
                row_bytes);
        }
    });
}

lcj_status lcj_bitmap_close(lcj_bitmap* bitmap)
{
    clear_error();
    delete bitmap;
    return LCJ_OK;
}

} // extern "C"
