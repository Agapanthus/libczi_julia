#include <libCZI/CZIReader.h>
#include <libCZI/libCZI.h>

#include <map>
#include <memory>
#include <string>
#include <vector>
#include <locale>
#include <codecvt>

struct MySubblockInfo {
    int64_t z; // Z position (−1 if not defined)
    int64_t c; // C position (−1 if not defined)
    int64_t t; // T position (−1 if not defined)
    int64_t r; // R position (−1 if not defined)
    int64_t s; // S position (−1 if not defined)
    int64_t i; // I position (−1 if not defined)
    int64_t h; // H position (−1 if not defined)
    int64_t v; // V position (−1 if not defined)
    int64_t b; // B position (−1 if not defined)

    int64_t mindex; // mosaic index (−1 if not mosaic)

    int64_t logical_x; // logical coordinates in the logical rectangle
    int64_t logical_y;
    int64_t logical_w;
    int64_t logical_h;

    int64_t physical_w; // physical size in pixels
    int64_t physical_h;

    uint64_t file_position; // file position of the subblock or
                            // std::numeric_limits<uint64_t>::max()
    int64_t subblock_index; // index of the subblock in the subblock directory
                            // (0-based)

    int32_t ptype;            // pixel format
    int32_t compression_mode; // compression mode
    int32_t pyramid_type;     // pyramid type

    MySubblockInfo(int subblock_index, const libCZI::DirectorySubBlockInfo &sb)
        : MySubblockInfo(subblock_index, sb.GetSubBlockInfo()) {
        this->file_position = sb.filePosition;
    }

    MySubblockInfo(int subblock_index, const libCZI::SubBlockInfo &sb) {
        {
            int z;
            if (sb.coordinate.GetPosition(libCZI::DimensionIndex::Z, &z))
                this->z = z;
            else
                this->z = -1;
            int c;
            if (sb.coordinate.GetPosition(libCZI::DimensionIndex::C, &c))
                this->c = c;
            else
                this->c = -1;
            int t;
            if (sb.coordinate.GetPosition(libCZI::DimensionIndex::T, &t))
                this->t = t;
            else
                this->t = -1;
            int r;
            if (sb.coordinate.GetPosition(libCZI::DimensionIndex::R, &r))
                this->r = r;
            else
                this->r = -1;
            int s;
            if (sb.coordinate.GetPosition(libCZI::DimensionIndex::S, &s))
                this->s = s;
            else
                this->s = -1;
            int i;
            if (sb.coordinate.GetPosition(libCZI::DimensionIndex::I, &i))
                this->i = i;
            else
                this->i = -1;
            int h;
            if (sb.coordinate.GetPosition(libCZI::DimensionIndex::H, &h))
                this->h = h;
            else
                this->h = -1;
            int v;
            if (sb.coordinate.GetPosition(libCZI::DimensionIndex::V, &v))
                this->v = v;
            else
                this->v = -1;
            int b;
            if (sb.coordinate.GetPosition(libCZI::DimensionIndex::B, &b))
                this->b = b;
            else
                this->b = -1;
        }

        this->mindex = sb.mIndex;
        if (!sb.IsMindexValid()) {
            this->mindex = -1; // mosaic index is not valid
        }
        this->logical_x = sb.logicalRect.x;
        this->logical_y = sb.logicalRect.y;
        this->logical_w = sb.logicalRect.w;
        this->logical_h = sb.logicalRect.h;
        this->physical_w = sb.physicalSize.w;
        this->physical_h = sb.physicalSize.h;
        this->ptype = static_cast<int32_t>(sb.pixelType);
        this->file_position = std::numeric_limits<uint64_t>::max();
        this->subblock_index = subblock_index;
        this->pyramid_type = static_cast<int32_t>(sb.pyramidType);
        this->compression_mode = sb.compressionModeRaw;
    }
};

struct MyGUID {
    uint32_t Data1;
    uint16_t Data2;
    uint16_t Data3;
    uint8_t Data4_0;
    uint8_t Data4_1;
    uint8_t Data4_2;
    uint8_t Data4_3;
    uint8_t Data4_4;
    uint8_t Data4_5;
    uint8_t Data4_6;
    uint8_t Data4_7;

    MyGUID(const libCZI::GUID &guid) {
        Data1 = guid.Data1;
        Data2 = guid.Data2;
        Data3 = guid.Data3;
        Data4_0 = guid.Data4[0];
        Data4_1 = guid.Data4[1];
        Data4_2 = guid.Data4[2];
        Data4_3 = guid.Data4[3];
        Data4_4 = guid.Data4[4];
        Data4_5 = guid.Data4[5];
        Data4_6 = guid.Data4[6];
        Data4_7 = guid.Data4[7];
    }
};

// Opaque wrapper around ISubBlock
class MySubblock {
    std::shared_ptr<libCZI::ISubBlock> subblock;
    const MySubblockInfo info;
    std::shared_ptr<libCZI::IBitmapData> bitmap_data;

  public:
    MySubblock(std::shared_ptr<libCZI::ISubBlock> subblock,
               const MySubblockInfo &info)
        : subblock(std::move(subblock)), info(info) {}

    MySubblock(std::shared_ptr<libCZI::ISubBlock> subblock, int subblock_index)
        : info(subblock_index, subblock->GetSubBlockInfo()) {
        this->subblock = std::move(subblock);
    }

    uint64_t decode_bitmap() {
        if (!bitmap_data) {
            bitmap_data = subblock->CreateBitmapData();
            if (!bitmap_data) {
                throw std::string("Failed to create bitmap data");
            }
        }
        return bmp->GetWidth() * bmp->GetHeight() *
               libCZI::Utils::GetBytesPerPixel(bmp->GetPixelType());
    }

    uint64_t copy_bitmap(uint8_t *buf, const uint64_t buffer_size) {
        const bytes = decode_bitmap();
        if (buffer_size < bytes) {
            return 0; // buffer too small
        }
        libCZI::ScopedBitmapLockerP lock{bmp.get()};
        std::memcpy(buf, lock.ptrDataRoi, bytes);
        return bytes; // number of bytes copied
    }

    uint64_t size(libCZI::ISubBlock block) const {
        std::size_t n = 0;
        void *_ptr;
        subblock->DangerousGetRawData(block, &_ptr, &n);
        return n; // size of metadata in bytes
    }

    uint64_t copy(libCZI::ISubBlock block, uint8_t *buf,
                  const uint64_t buffer_size) const {
        std::size_t n = 0;
        void *ptr;
        subblock->DangerousGetRawData(block, &ptr, &n);
        if (buffer_size < n) {
            return 0; // buffer too small
        }
        if (n > 0) {
            std::memcpy(buf, ptr, n);
        }
        return n; // size of metadata in bytes
    }
};

/*
// Opaque wrapper around AttachmentInfo
class MyAttachmentInfo {
    MyGUID contentGuid; // A Guid identifying the content of the attachment.
    char contentFileType[9]; // A null-terminated character array identifying
                             // the content of the attachment.
    std::string name; // A string identifying the content of the attachment.
    int index;

  public:
    MyAttachmentInfo() = default;

    MyAttachmentInfo(int index, const libCZI::AttachmentInfo &info)
        : contentGuid(MyGUID(info.contentGuid)),
          contentFileType{0}, // initialize to null-terminated
          name(info.name), index(index) {
        std::strncpy(contentFileType, info.contentFileType, 8);
        contentFileType[8] = '\0'; // ensure null-termination
    }
};
*/
struct DimensionRanges {
    int64_t z_start;
    int64_t z_end;

    int64_t c_start;
    int64_t c_end;

    int64_t t_start;
    int64_t t_end;

    int64_t r_start;
    int64_t r_end;

    int64_t s_start;
    int64_t s_end;

    int64_t i_start;
    int64_t i_end;

    int64_t h_start;
    int64_t h_end;

    int64_t v_start;
    int64_t v_end;

    int64_t b_start;
    int64_t b_end;

    int64_t m_start;
    int64_t m_end;

    dimension_ranges()
        : z_start(0), z_end(-1), c_start(0), c_end(-1), t_start(0), t_end(-1),
          r_start(0), r_end(-1), s_start(0), s_end(-1), i_start(0), i_end(-1),
          h_start(0), h_end(-1), v_start(0), v_end(-1), b_start(0), b_end(-1),
          m_start(0), m_end(-1) {}
}

struct SceneBoundingBox {
    int64_t x;
    int64_t y;
    int64_t w;
    int64_t h;

    int64_t x0;
    int64_t y0;
    int64_t w0;
    int64_t h0;
}

// Opaque wrapper around CCZIReader
class MyCziFile {
    std::shared_ptr<CCZIReader> reader; // owning reader
    libCZI::SubBlockStatistics stats;   // dim & bbox summary

  public:
    explicit MyCziFile(const std::string &path) {
        reader = std::make_shared<CCZIReader>();
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        auto strm =
            libCZI::CreateStreamFromFile(converter.from_bytes(path).c_str());
        reader->Open(strm, nullptr);
        stats = reader->GetStatistics();
    }

    ~MyCziFile() {
        if (reader) {
            reader->Close();
            reader.reset();
        }
    }

    DimensionRanges dimension_ranges() const {
        DimensionRanges dr;
        {
            int p, s;
            if (stats.dimBounds.TryGetInterval(libCZI::DimensionIndex::Z, &p,
                                               &s)) {
                dr.z_start = p;
                dr.z_end = p + s;
            }
            if (stats.dimBounds.TryGetInterval(libCZI::DimensionIndex::C, &p,
                                               &s)) {
                dr.c_start = p;
                dr.c_end = p + s;
            }
            if (stats.dimBounds.TryGetInterval(libCZI::DimensionIndex::T, &p,
                                               &s)) {
                dr.t_start = p;
                dr.t_end = p + s;
            }
            if (stats.dimBounds.TryGetInterval(libCZI::DimensionIndex::R, &p,
                                               &s)) {
                dr.r_start = p;
                dr.r_end = p + s;
            }
            if (stats.dimBounds.TryGetInterval(libCZI::DimensionIndex::S, &p,
                                               &s)) {
                dr.s_start = p;
                dr.s_end = p + s;
            }
            if (stats.dimBounds.TryGetInterval(libCZI::DimensionIndex::I, &p,
                                               &s)) {
                dr.i_start = p;
                dr.i_end = p + s;
            }
            if (stats.dimBounds.TryGetInterval(libCZI::DimensionIndex::H, &p,
                                               &s)) {
                dr.h_start = p;
                dr.h_end = p + s;
            }
            if (stats.dimBounds.TryGetInterval(libCZI::DimensionIndex::V, &p,
                                               &s)) {
                dr.v_start = p;
                dr.v_end = p + s;
            }
            if (stats.dimBounds.TryGetInterval(libCZI::DimensionIndex::B, &p,
                                               &s)) {
                dr.b_start = p;
                dr.b_end = p + s;
            }
        }

        if (stats.maxMindex != std::numeric_limits<int>::min()) {
            dr.m_start = stats.minMindex;
            dr.m_end = stats.maxMindex + 1; // exclusive end
        }

        return dr;
    }

    bool copy_scene_bounding_box(SceneBoundingBox *bbox, int s) const {
        if (!stats.sceneBoundingBoxes.contains(s)) {
            return false;
        }
        const boxes = stats.sceneBoundingBoxes[s];
        const libCZI::IntRect b = boxes.boundingBox;
        bbox->x = b.x;
        bbox->y = b.y;
        bbox->w = b.w;
        bbox->h = b.h;
        const libCZI::IntRect b0 = boxes.boundingBoxLayer0;
        bbox->x0 = b0.x;
        bbox->y0 = b0.y;
        bbox->w0 = b0.w;
        bbox->h0 = b0.h;
        return true;
    }

    uint64_t subblock_count() const { return stats->subBlockCount; }

    uint64_t copy_subblocks(MySubblockInfo *info, const int buffer_size) const {
        int pos = 0;
        reader->EnumerateSubBlocksEx(
            [&](int idx, const libCZI::SubBlockInfo &sb) {
                if (pos >= buffer_size) {
                    return false; // stop enumeration if buffer is full
                }
                info[pos] = MySubblockInfo(idx, sb);
                pos++;
                return true;
            });
        return pos;
    }

    /*std::vector<MySubblockInfo> subblocks_level0() const {
        std::vector<MySubblockInfo> v;
        reader->EnumerateSubBlocksEx(
            [&](int idx, const libCZI::SubBlockInfo &sb) {
                if (sb.pyramidType != libCZI::SubBlockPyramidType::None) {
                    return true;
                }
                v.push_back(MySubblockInfo(idx, sb));
                return true;
            });
        return v;
    }*/

    MySubblock *subblock(const int subblock_index) const {
        auto sb = reader->ReadSubBlock(subblock_index);
        if (!sb) {
            return nullptr; // subblock not found
        }
        return new MySubblock(sb, subblock_index);
    }

    std::string metadata() const {
        auto seg = reader->ReadMetadataSegment();
        auto meta = seg->CreateMetaFromMetadataSegment();
        return meta->GetXml(); // UTF-8
    }

    MyGUID file_guid() const {
        auto info = reader->GetFileHeaderInfo();
        return MyGUID(info.fileGuid);
    }

    int major_version() const {
        auto info = reader->GetFileHeaderInfo();
        return info.majorVersion;
    }

    int minor_version() const {
        auto info = reader->GetFileHeaderInfo();
        return info.minorVersion;
    }

    /*std::vector<MyAttachmentInfo> attachments() const {
        std::vector<MyAttachmentInfo> v;
        reader->EnumerateAttachments(
            [&](int idx, const libCZI::AttachmentInfo &info) {
                v.push_back(MyAttachmentInfo(idx, info));
                return true;
            });
        return v;
    }

    std::vector<std::uint8_t> attachment(const int index) const {
        auto att = reader->ReadAttachment(index);
        if (!att) {
            throw std::string("Attachment not found or could not be read");
        }
        std::size_t n = 0;
        auto mem = att->GetRawData(&n);
        if (n == 0) {
            return {};
        }
        std::vector<std::uint8_t> buf(n);
        std::memcpy(buf.data(), mem.get(), n);
        return buf;
    }*/
};

// Julia FFI interface
extern "C" {

void subblock_free(MySubblock *subblock) { delete subblock; }

uint64_t subblock_decode_bitmap(MySubblock *subblock) {
    return subblock->decode_bitmap();
}

uint64_t subblock_copy_bitmap(MySubblock *subblock, uint8_t *buf,
                              const uint64_t buffer_size) {
    return subblock->copy_bitmap(buf, buffer_size);
}

uint64_t subblock_meta_size(const MySubblock *subblock) {
    return subblock->size(libCZI::ISubBlock::Metadata);
}

uint64_t subblock_meta_copy(const MySubblock *subblock, uint8_t *buf,
                            const uint64_t buffer_size) {
    return subblock->copy(libCZI::ISubBlock::Metadata, buf, buffer_size);
}

uint64_t subblock_attachment_size(const MySubblock *subblock) {
    return subblock->size(libCZI::ISubBlock::Attachment);
}

uint64_t subblock_attachment_copy(const MySubblock *subblock, uint8_t *buf,
                                  const uint64_t buffer_size) {
    return subblock->copy(libCZI::ISubBlock::Attachment, buf, buffer_size);
}

/*
void attachment_info_free(MyAttachmentInfo *info) { delete info; }

MyGUID attachment_info_guid(const MyAttachmentInfo *info) {
    return info->contentGuid;
}

const char *attachment_info_file_type(const MyAttachmentInfo *info) {
    return info->contentFileType;
}

const char *attachment_info_name(const MyAttachmentInfo *info) {
    return info->name.c_str();
}

int attachment_info_index(const MyAttachmentInfo *info) { return info->index; }
*/

MyCziFile *czi_open(const char *path) { return new MyCziFile(path); }

void czi_close(MyCziFile *czi) { delete czi; }

DimensionRanges czi_dimension_ranges(const MyCziFile *czi) {
    return czi->dimension_ranges();
}

bool czi_copy_scene_bounding_box(const MyCziFile *czi, SceneBoundingBox *bbox,
                                 int s) {
    return czi->copy_scene_bounding_box(bbox, s);
}

uint64_t czi_subblock_count(const MyCziFile *czi) {
    return czi->subblock_count();
}

uint64_t czi_copy_subblocks(const MyCziFile *czi, MySubblockInfo *info,
                            const int buffer_size) {
    return czi->copy_subblocks(info, buffer_size);
}

MySubblock *czi_subblock(const MyCziFile *czi, const int subblock_index) {
    return czi->subblock(subblock_index);
}

uint64_t czi_metadata_length(const MyCziFile *czi) {
    return czi->metadata().length();
}

uint64_t czi_metadata_copy(const MyCziFile *czi, char *buf,
                           const uint64_t buffer_size) {
    std::string meta = czi->metadata();
    if (buffer_size < meta.length()) {
        return 0; // buffer too small
    }
    std::memcpy(buf, meta.c_str(), meta.length());
    return meta.length(); // number of bytes copied
}

MyGUID czi_file_guid(const MyCziFile *czi) { return czi->file_guid(); }

int czi_major_version(const MyCziFile *czi) { return czi->major_version(); }

int czi_minor_version(const MyCziFile *czi) { return czi->minor_version(); }
}