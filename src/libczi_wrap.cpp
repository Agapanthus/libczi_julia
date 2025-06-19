#include <jlcxx/jlcxx.hpp>
#include <jlcxx/array.hpp>
#include <jlcxx/tuple.hpp>
#include <jlcxx/stl.hpp>

#include <libCZI/CZIReader.h>
#include <libCZI/libCZI.h>

#include <map>
#include <memory>
#include <string>
#include <vector>
#include <locale>
#include <codecvt>

class MySubblockInfo {
    libCZI::CDimCoordinate coords; // all defined dimension positions
    int mindex;                    // mosaic index (−1 if not mosaic)
    libCZI::IntRect logical;       // logical (x,y,w,h) in specimen pixels
    libCZI::IntSize physical;      // physical size
    libCZI::PixelType ptype;       // pixel format
    uint64_t file_position;        // file position of the subblock or
                                   // std::numeric_limits<uint64_t>::max()
    int subblock_index; // index of the subblock in the subblock directory
                        // (0-based)
    libCZI::CompressionMode compression_mode;
    libCZI::SubBlockPyramidType pyramid_type;

  public:
    MySubblockInfo() = default;

    MySubblockInfo(int subblock_index,
                   const libCZI::DirectorySubBlockInfo &sb) {
        this->coords = sb.coordinate;
        this->mindex = sb.mIndex;
        if (!sb.IsMindexValid()) {
            this->mindex = -1; // mosaic index is not valid
        }
        this->logical = sb.logicalRect;
        this->physical = sb.physicalSize;
        this->ptype = sb.pixelType;
        this->file_position = sb.filePosition;
        this->subblock_index = subblock_index;
        this->pyramid_type = sb.pyramidType;
        this->compression_mode = sb.GetCompressionMode();
    }

    MySubblockInfo(int subblock_index, const libCZI::SubBlockInfo &sb) {
        this->coords = sb.coordinate;
        this->mindex = sb.mIndex;
        if (!sb.IsMindexValid()) {
            this->mindex = -1; // mosaic index is not valid
        }
        this->logical = sb.logicalRect;
        this->physical = sb.physicalSize;
        this->ptype = sb.pixelType;
        this->file_position = std::numeric_limits<uint64_t>::max();
        this->subblock_index = subblock_index;
        this->pyramid_type = sb.pyramidType;
        this->compression_mode = sb.GetCompressionMode();
    }

    uint64_t file_pos() const { return file_position; }

    int index() const { return subblock_index; }

    libCZI::PixelType type() const { return ptype; }

    std::tuple<int, int, int, int> get_logical() const {
        return std::tuple{logical.x, logical.y, logical.w, logical.h};
    }

    std::tuple<int, int> get_physical() const {
        return std::tuple{physical.w, physical.h};
    }

    libCZI::SubBlockPyramidType pyramid() const { return pyramid_type; }

    libCZI::CompressionMode compression() const { return compression_mode; }

    int m() const { return mindex; }

    int z() const {
        int val;
        if (coords.TryGetPosition(libCZI::DimensionIndex::Z, &val))
            return val;
        else
            return -1;
    }

    int c() const {
        int val;
        if (coords.TryGetPosition(libCZI::DimensionIndex::C, &val))
            return val;
        else
            return -1;
    }

    int t() const {
        int val;
        if (coords.TryGetPosition(libCZI::DimensionIndex::T, &val))
            return val;
        else
            return -1;
    }

    int r() const {
        int val;
        if (coords.TryGetPosition(libCZI::DimensionIndex::R, &val))
            return val;
        else
            return -1;
    }

    int s() const {
        int val;
        if (coords.TryGetPosition(libCZI::DimensionIndex::S, &val))
            return val;
        else
            return -1;
    }

    int i() const {
        int val;
        if (coords.TryGetPosition(libCZI::DimensionIndex::I, &val))
            return val;
        else
            return -1;
    }

    int h() const {
        int val;
        if (coords.TryGetPosition(libCZI::DimensionIndex::H, &val))
            return val;
        else
            return -1;
    }

    int v() const {
        int val;
        if (coords.TryGetPosition(libCZI::DimensionIndex::V, &val))
            return val;
        else
            return -1;
    }

    int b() const {
        int val;
        if (coords.TryGetPosition(libCZI::DimensionIndex::B, &val))
            return val;
        else
            return -1;
    }
};

class MySubblock {
    std::shared_ptr<libCZI::ISubBlock> subblock;
    const MySubblockInfo info;

  public:
    MySubblock(std::shared_ptr<libCZI::ISubBlock> subblock,
               const MySubblockInfo &info)
        : subblock(std::move(subblock)), info(info) {}

    MySubblock(std::shared_ptr<libCZI::ISubBlock> subblock, int subblock_index)
        : info(subblock_index, subblock->GetSubBlockInfo()) {
        this->subblock = std::move(subblock);
    }

    std::vector<std::uint8_t> bitmap() const {
        auto bmp = subblock->CreateBitmap(); // decoded pixels
        const auto size = bmp->GetSize();
        const std::size_t bytes =
            bmp->GetWidth() * bmp->GetHeight() *
            libCZI::Utils::GetBytesPerPixel(bmp->GetPixelType());
        libCZI::ScopedBitmapLockerP lock{bmp.get()};
        std::vector<std::uint8_t> buf(bytes);
        std::memcpy(buf.data(), lock.ptrDataRoi, bytes);
        return buf;
    }

    std::string meta() const {
        std::size_t n = 0;
        auto mem = subblock->GetRawData(libCZI::ISubBlock::Metadata, &n);
        return std::string(reinterpret_cast<const char *>(mem.get()), n);
    }

    std::string attachment() const {
        std::size_t n = 0;
        auto mem = subblock->GetRawData(libCZI::ISubBlock::Attachment, &n);
        return std::string(reinterpret_cast<const char *>(mem.get()), n);
    }
};

struct MyGUID {
    std::uint32_t Data1;
    std::uint16_t Data2;
    std::uint16_t Data3;
    std::uint8_t Data4[8];

    MyGUID() : Data1(0), Data2(0), Data3(0) {
        std::fill(std::begin(Data4), std::end(Data4), 0);
    }

    MyGUID(const libCZI::GUID &guid) {
        Data1 = guid.Data1;
        Data2 = guid.Data2;
        Data3 = guid.Data3;
        std::copy(std::begin(guid.Data4), std::end(guid.Data4),
                  std::begin(Data4));
    }

    std::string to_string() const {
        char buffer[37];
        snprintf(buffer, sizeof(buffer),
                 "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x", Data1,
                 Data2, Data3, Data4[0], Data4[1], Data4[2], Data4[3], Data4[4],
                 Data4[5], Data4[6], Data4[7]);
        return std::string(buffer);
    }

    bool operator==(const MyGUID &other) const {
        return Data1 == other.Data1 && Data2 == other.Data2 &&
               Data3 == other.Data3 &&
               std::equal(std::begin(Data4), std::end(Data4),
                          std::begin(other.Data4));
    }

    std::uint32_t data1() const { return Data1; }
    std::uint16_t data2() const { return Data2; }
    std::uint16_t data3() const { return Data3; }
    std::vector<std::uint8_t> data4() const {
        return std::vector<std::uint8_t>(Data4, Data4 + 8);
    }
};

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

    MyGUID get_content_guid() const { return contentGuid; }
    std::string get_content_file_type() const {
        return std::string(contentFileType);
    }
    std::string get_name() const { return name; }
    int get_index() const { return index; }
};

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

    std::vector<std::tuple<char, int, int>> dimension_ranges() const {
        std::map<char, std::pair<int, int>> out; // {dimChar → [start,end)}
        stats.dimBounds.EnumValidDimensions(
            [&](libCZI::DimensionIndex d, int s, int n) {
                out.emplace(libCZI::Utils::DimensionToChar(d),
                            std::make_pair(s, s + n));
                return true;
            });
        if (stats.maxMindex != std::numeric_limits<int>::min())
            out.emplace(std::pair{
                'M', std::pair{stats.minMindex, stats.maxMindex + 1}});
        out.emplace(
            std::pair{'Y', std::pair{0, stats.boundingBoxLayer0Only.h}});
        out.emplace(
            std::pair{'X', std::pair{0, stats.boundingBoxLayer0Only.w}});

        std::vector<std::tuple<char, int, int>> arr;
        for (const auto el : out) {
            arr.push_back(
                std::tuple{el.first, el.second.first, el.second.second});
        }
        return arr;
    }

    std::vector<MySubblockInfo> subblocks() const {
        std::vector<MySubblockInfo> v;
        reader->EnumerateSubBlocksEx(
            [&](int idx, const libCZI::SubBlockInfo &sb) {
                v.push_back(MySubblockInfo(idx, sb));
                return true;
            });
        return v;
    }

    std::vector<MySubblockInfo> subblocks_level0() const {
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
    }

    MySubblock subblock(const int subblock_index) const {
        auto sb = reader->ReadSubBlock(subblock_index);
        if (!sb) {
            throw std::string("Subblock not found or could not be read");
        }
        return MySubblock(sb, subblock_index);
    }

    std::string metadata() const {
        auto seg = reader->ReadMetadataSegment();
        auto meta = seg->CreateMetaFromMetadataSegment();
        return meta->GetXml(); // UTF-8
    }

    std::tuple<MyGUID, int, int> header() const {
        auto info = reader->GetFileHeaderInfo();
        return std::make_tuple(MyGUID(info.fileGuid), info.majorVersion,
                               info.minorVersion);
    }

    std::vector<MyAttachmentInfo> attachments() const {
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
    }
};

JLCXX_MODULE define_julia_module(jlcxx::Module &mod) {

    mod.add_bits<libCZI::CompressionMode>("CompressionMode",
                                          jlcxx::julia_type("CppEnum"));
    mod.set_const("CompressionModeInvalid", libCZI::CompressionMode::Invalid);
    mod.set_const("CompressionModeUncompressed",
                  libCZI::CompressionMode::UnCompressed);
    mod.set_const("CompressionModeJpg", libCZI::CompressionMode::Jpg);
    mod.set_const("CompressionModeJpgXr", libCZI::CompressionMode::JpgXr);
    mod.set_const("CompressionModeZstd0", libCZI::CompressionMode::Zstd0);
    mod.set_const("CompressionModeZstd1", libCZI::CompressionMode::Zstd1);

    mod.add_bits<libCZI::SubBlockPyramidType>("SubBlockPyramidType",
                                              jlcxx::julia_type("CppEnum"));
    mod.set_const("SubBlockPyramidTypeInvalid",
                  libCZI::SubBlockPyramidType::Invalid);
    mod.set_const("SubBlockPyramidTypeNone", libCZI::SubBlockPyramidType::None);
    mod.set_const("SubBlockPyramidTypeSingleSubBlock",
                  libCZI::SubBlockPyramidType::SingleSubBlock);
    mod.set_const("SubBlockPyramidTypeMultiSubBlock",
                  libCZI::SubBlockPyramidType::MultiSubBlock);

    mod.add_bits<libCZI::PixelType>("PixelType", jlcxx::julia_type("CppEnum"));
    mod.set_const("PixelTypeInvalid", libCZI::PixelType::Invalid);
    mod.set_const("PixelTypeGray8", libCZI::PixelType::Gray8);
    mod.set_const("PixelTypeGray16", libCZI::PixelType::Gray16);
    mod.set_const("PixelTypeGray32Float", libCZI::PixelType::Gray32Float);
    mod.set_const("PixelTypeBgr24", libCZI::PixelType::Bgr24);
    mod.set_const("PixelTypeBgr48", libCZI::PixelType::Bgr48);
    mod.set_const("PixelTypeBgr96Float", libCZI::PixelType::Bgr96Float);
    mod.set_const("PixelTypeBgra32", libCZI::PixelType::Bgra32);
    mod.set_const("PixelTypeGray64ComplexFloat",
                  libCZI::PixelType::Gray64ComplexFloat);
    mod.set_const("PixelTypeBgr192ComplexFloat",
                  libCZI::PixelType::Bgr192ComplexFloat);
    mod.set_const("PixelTypeGray32", libCZI::PixelType::Gray32);
    mod.set_const("PixelTypeGray64Float", libCZI::PixelType::Gray64Float);

    mod.add_type<MySubblockInfo>("SubblockInfo")
        .method("type", &MySubblockInfo::type)
        .method("file_pos", &MySubblockInfo::file_pos)
        .method("physical",
                [](const MySubblockInfo &sb) -> std::tuple<int, int> {
                    return sb.get_physical();
                })
        .method("logical",
                [](const MySubblockInfo &sb) -> std::tuple<int, int, int, int> {
                    return sb.get_logical();
                })
        .method("index", &MySubblockInfo::index)
        .method("pyramid_type", &MySubblockInfo::pyramid)
        .method("compression", &MySubblockInfo::compression)
        .method("m_index", &MySubblockInfo::m)
        .method("z_index", &MySubblockInfo::z)
        .method("c_index", &MySubblockInfo::c)
        .method("t_index", &MySubblockInfo::t)
        .method("r_index", &MySubblockInfo::r)
        .method("s_index", &MySubblockInfo::s)
        .method("i_index", &MySubblockInfo::i)
        .method("h_index", &MySubblockInfo::h)
        .method("v_index", &MySubblockInfo::v)
        .method("b_index", &MySubblockInfo::b);

    mod.add_type<MyGUID>("GUID")
        .method("to_string", &MyGUID::to_string)
        .method("data1", &MyGUID::data1)
        .method("data2", &MyGUID::data2)
        .method("data3", &MyGUID::data3)
        .method("data4", &MyGUID::data4);

    mod.method("initialize_vector_of_subblocks_type",
               [](const std::vector<MySubblockInfo> info) -> int {
                   return info.size();
               });

    mod.add_type<MyAttachmentInfo>("AttachmentInfo")
        .method("get_content_guid", &MyAttachmentInfo::get_content_guid)
        .method("get_content_file_type",
                &MyAttachmentInfo::get_content_file_type)
        .method("get_name", &MyAttachmentInfo::get_name)
        .method("get_index", &MyAttachmentInfo::get_index);

    mod.add_type<MySubblock>("Subblock")
        .method("bitmap",
                [](const MySubblock &sb) -> std::vector<std::uint8_t> {
                    return sb.bitmap();
                })
        .method("meta",
                [](const MySubblock &sb) -> std::string { return sb.meta(); })
        .method("attachment", [](const MySubblock &sb) -> std::string {
            return sb.attachment();
        });

    mod.add_type<MyCziFile>("CziFile")
        .constructor<const std::string &>()
        .method(
            "dimension_ranges",
            [](const MyCziFile &p) -> std::vector<std::tuple<char, int, int>> {
                return p.dimension_ranges();
            })
        .method("subblocks",
                [](const MyCziFile &p) -> std::vector<MySubblockInfo> {
                    return p.subblocks();
                })
        .method("subblocks_level0",
                [](const MyCziFile &p) -> std::vector<MySubblockInfo> {
                    return p.subblocks_level0();
                })
        .method("subblock",
                [](const MyCziFile &p, int subblock_index) {
                    return p.subblock(subblock_index);
                })
        .method("metadata",
                [](const MyCziFile &p) -> std::string { return p.metadata(); })
        .method("header",
                [](const MyCziFile &p) -> std::tuple<MyGUID, int, int> {
                    return p.header();
                })
        .method("attachments",
                [](const MyCziFile &p) -> std::vector<MyAttachmentInfo> {
                    return p.attachments();
                })
        .method("attachment",
                [](const MyCziFile &p, int index) -> std::vector<std::uint8_t> {
                    return p.attachment(index);
                });
}