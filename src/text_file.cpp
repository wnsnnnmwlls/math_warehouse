#include "text_file.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <limits>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace {

constexpr uint64_t kMaxInputFileSize = 512ULL * 1024ULL * 1024ULL;
constexpr uint64_t kMaxDocumentXmlSize = 256ULL * 1024ULL * 1024ULL;

constexpr uint32_t kZipLocalSignature = 0x04034B50u;
constexpr uint32_t kZipCentralSignature = 0x02014B50u;
constexpr uint32_t kZipEndSignature = 0x06054B50u;

// ---------------------------------------------------------------------------
// Small helpers
// ---------------------------------------------------------------------------

std::string PathUtf8(const std::filesystem::path& path) {
    try {
        std::string value = path.u8string();
        if (!value.empty()) return value;
    } catch (...) {
    }
    try {
        return path.string();
    } catch (...) {
    }
    return "(未知路径)";
}

std::string ExtensionLower(const std::filesystem::path& path) {
    std::string extension = path.extension().u8string();
    for (char& ch : extension) {
        if (ch >= 'A' && ch <= 'Z') ch = static_cast<char>(ch - 'A' + 'a');
    }
    return extension;
}

std::string WithPath(const std::filesystem::path& path, const std::string& detail) {
    return PathUtf8(path) + ": " + detail;
}

std::string Number(uint64_t value) { return std::to_string(value); }

bool ReadWholeFile(const std::filesystem::path& path, std::string& bytes, std::string& error) {
    std::error_code ec;
    const bool exists = std::filesystem::exists(path, ec);
    if (ec) {
        error = "无法访问文件（" + ec.message() + "）";
        return false;
    }
    if (!exists) {
        error = "文件不存在";
        return false;
    }

    ec.clear();
    const bool regular = std::filesystem::is_regular_file(path, ec);
    if (ec) {
        error = "无法读取文件属性（" + ec.message() + "）";
        return false;
    }
    if (!regular) {
        error = "路径不是普通文件";
        return false;
    }

    std::ifstream input(path, std::ios::binary);
    if (!input) {
        error = "无法打开文件进行读取";
        return false;
    }

    input.seekg(0, std::ios::end);
    const std::streamoff end = input.tellg();
    if (end < 0) {
        error = "无法确定文件大小";
        return false;
    }
    const uint64_t size = static_cast<uint64_t>(end);
    if (size > kMaxInputFileSize) {
        error = "文件过大（" + Number(size) + " 字节），上限为 " +
                Number(kMaxInputFileSize / (1024ULL * 1024ULL)) + " MB";
        return false;
    }

    std::string data(static_cast<size_t>(size), '\0');
    input.seekg(0, std::ios::beg);
    if (!input) {
        error = "无法定位到文件开头";
        return false;
    }
    if (!data.empty()) {
        input.read(&data[0], static_cast<std::streamsize>(data.size()));
        if (!input || input.gcount() != static_cast<std::streamsize>(data.size())) {
            error = "读取文件失败（文件可能被其他程序占用或已更改）";
            return false;
        }
    }
    bytes = std::move(data);
    return true;
}

// ---------------------------------------------------------------------------
// UTF-8 / UTF-16 helpers
// ---------------------------------------------------------------------------

bool DecodeUtf8CodePoint(std::string_view text, size_t& index, uint32_t& codePoint) {
    if (index >= text.size()) return false;
    const unsigned char first = static_cast<unsigned char>(text[index]);
    if (first < 0x80) {
        codePoint = first;
        ++index;
        return true;
    }

    int extra = 0;
    uint32_t value = 0;
    if ((first & 0xE0u) == 0xC0u) {
        extra = 1;
        value = first & 0x1Fu;
    } else if ((first & 0xF0u) == 0xE0u) {
        extra = 2;
        value = first & 0x0Fu;
    } else if ((first & 0xF8u) == 0xF0u) {
        extra = 3;
        value = first & 0x07u;
    } else {
        return false;
    }

    if (index + static_cast<size_t>(extra) >= text.size()) return false;
    for (int i = 1; i <= extra; ++i) {
        const unsigned char next = static_cast<unsigned char>(text[index + static_cast<size_t>(i)]);
        if ((next & 0xC0u) != 0x80u) return false;
        value = (value << 6) | (next & 0x3Fu);
    }

    if (extra == 2 && value < 0x800u) return false;
    if (extra == 3 && value < 0x10000u) return false;
    if (value > 0x10FFFFu) return false;
    if (value >= 0xD800u && value <= 0xDFFFu) return false;

    codePoint = value;
    index += static_cast<size_t>(extra) + 1;
    return true;
}

bool ValidateUtf8(std::string_view text, size_t& badOffset) {
    size_t index = 0;
    while (index < text.size()) {
        const size_t start = index;
        uint32_t codePoint = 0;
        if (!DecodeUtf8CodePoint(text, index, codePoint)) {
            badOffset = start;
            return false;
        }
    }
    return true;
}

void AppendUtf8(std::string& out, uint32_t codePoint) {
    if (codePoint <= 0x7Fu) {
        out.push_back(static_cast<char>(codePoint));
    } else if (codePoint <= 0x7FFu) {
        out.push_back(static_cast<char>(0xC0u | (codePoint >> 6)));
        out.push_back(static_cast<char>(0x80u | (codePoint & 0x3Fu)));
    } else if (codePoint <= 0xFFFFu) {
        out.push_back(static_cast<char>(0xE0u | (codePoint >> 12)));
        out.push_back(static_cast<char>(0x80u | ((codePoint >> 6) & 0x3Fu)));
        out.push_back(static_cast<char>(0x80u | (codePoint & 0x3Fu)));
    } else {
        out.push_back(static_cast<char>(0xF0u | (codePoint >> 18)));
        out.push_back(static_cast<char>(0x80u | ((codePoint >> 12) & 0x3Fu)));
        out.push_back(static_cast<char>(0x80u | ((codePoint >> 6) & 0x3Fu)));
        out.push_back(static_cast<char>(0x80u | (codePoint & 0x3Fu)));
    }
}

bool Utf16ToUtf8(std::string_view bytes, bool littleEndian, std::string& out,
                 std::string& error) {
    if ((bytes.size() % 2) != 0) {
        error = "UTF-16 字节数为奇数，文件可能已损坏";
        return false;
    }
    out.clear();
    out.reserve(bytes.size());

    for (size_t i = 0; i < bytes.size(); i += 2) {
        const unsigned char first = static_cast<unsigned char>(bytes[i]);
        const unsigned char second = static_cast<unsigned char>(bytes[i + 1]);
        const uint16_t unit = littleEndian
                                  ? static_cast<uint16_t>(first | (static_cast<uint16_t>(second) << 8))
                                  : static_cast<uint16_t>(second | (static_cast<uint16_t>(first) << 8));

        if (unit >= 0xD800u && unit <= 0xDBFFu) {
            if (i + 3 >= bytes.size()) {
                error = "UTF-16 代理项不完整（字节偏移 " + Number(i) + "）";
                return false;
            }
            const unsigned char third = static_cast<unsigned char>(bytes[i + 2]);
            const unsigned char fourth = static_cast<unsigned char>(bytes[i + 3]);
            const uint16_t low =
                littleEndian
                    ? static_cast<uint16_t>(third | (static_cast<uint16_t>(fourth) << 8))
                    : static_cast<uint16_t>(fourth | (static_cast<uint16_t>(third) << 8));
            if (low < 0xDC00u || low > 0xDFFFu) {
                error = "UTF-16 代理项无效（字节偏移 " + Number(i) + "）";
                return false;
            }
            const uint32_t codePoint =
                0x10000u + ((static_cast<uint32_t>(unit) - 0xD800u) << 10) +
                (static_cast<uint32_t>(low) - 0xDC00u);
            AppendUtf8(out, codePoint);
            i += 2;
        } else if (unit >= 0xDC00u && unit <= 0xDFFFu) {
            error = "UTF-16 中出现孤立的低位代理项（字节偏移 " + Number(i) + "）";
            return false;
        } else {
            AppendUtf8(out, unit);
        }
    }
    return true;
}

void NormalizeNewlinesInPlace(std::string& text) {
    size_t write = 0;
    for (size_t read = 0; read < text.size(); ++read) {
        const char ch = text[read];
        if (ch == '\r') {
            text[write++] = '\n';
            if (read + 1 < text.size() && text[read + 1] == '\n') ++read;
        } else {
            text[write++] = ch;
        }
    }
    text.resize(write);
}

bool DecodePlainText(std::string_view bytes, std::string& text, std::string& error) {
    if (bytes.size() >= 2 && static_cast<unsigned char>(bytes[0]) == 0xFFu &&
        static_cast<unsigned char>(bytes[1]) == 0xFEu) {
        std::string detail;
        if (!Utf16ToUtf8(bytes.substr(2), true, text, detail)) {
            error = "无法解码 UTF-16LE 文本：" + detail;
            return false;
        }
    } else if (bytes.size() >= 2 && static_cast<unsigned char>(bytes[0]) == 0xFEu &&
               static_cast<unsigned char>(bytes[1]) == 0xFFu) {
        std::string detail;
        if (!Utf16ToUtf8(bytes.substr(2), false, text, detail)) {
            error = "无法解码 UTF-16BE 文本：" + detail;
            return false;
        }
    } else {
        size_t offset = 0;
        if (bytes.size() >= 3 && static_cast<unsigned char>(bytes[0]) == 0xEFu &&
            static_cast<unsigned char>(bytes[1]) == 0xBBu &&
            static_cast<unsigned char>(bytes[2]) == 0xBFu) {
            offset = 3;
        }
        const std::string_view content = bytes.substr(offset);
        size_t badOffset = 0;
        if (!ValidateUtf8(content, badOffset)) {
            error = "文本不是有效的 UTF-8（字节偏移 " + Number(offset + badOffset) +
                    "）；请将文件另存为 UTF-8、UTF-8 BOM 或 UTF-16LE 后重试";
            return false;
        }
        text.assign(content);
    }
    NormalizeNewlinesInPlace(text);
    return true;
}

// ---------------------------------------------------------------------------
// CRC-32 and little-endian field readers
// ---------------------------------------------------------------------------

uint16_t ReadLe16(const unsigned char* data) {
    return static_cast<uint16_t>(static_cast<uint16_t>(data[0]) |
                                 (static_cast<uint16_t>(data[1]) << 8));
}

uint32_t ReadLe32(const unsigned char* data) {
    return static_cast<uint32_t>(data[0]) | (static_cast<uint32_t>(data[1]) << 8) |
           (static_cast<uint32_t>(data[2]) << 16) | (static_cast<uint32_t>(data[3]) << 24);
}

uint64_t ReadLe64(const unsigned char* data) {
    return static_cast<uint64_t>(ReadLe32(data)) |
           (static_cast<uint64_t>(ReadLe32(data + 4)) << 32);
}

uint32_t Crc32(const unsigned char* data, size_t size) {
    static const std::array<uint32_t, 256> table = [] {
        std::array<uint32_t, 256> values{};
        for (uint32_t i = 0; i < 256; ++i) {
            uint32_t value = i;
            for (int bit = 0; bit < 8; ++bit) {
                value = (value & 1u) ? (0xEDB88320u ^ (value >> 1)) : (value >> 1);
            }
            values[i] = value;
        }
        return values;
    }();

    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < size; ++i) {
        crc = table[(crc ^ static_cast<uint32_t>(data[i])) & 0xFFu] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFFu;
}

// ---------------------------------------------------------------------------
// Minimal raw-DEFLATE decoder (puff-style, RFC 1951)
// ---------------------------------------------------------------------------

class BitReader {
public:
    BitReader(const unsigned char* data, size_t size)
        : data_(data), bitSize_(size * 8), bitPosition_(0) {}

    bool ReadBits(unsigned count, uint32_t& value) {
        if (count > 24) return false;
        if (count > bitSize_ - bitPosition_) return false;
        uint32_t result = 0;
        for (unsigned i = 0; i < count; ++i) {
            const size_t byteIndex = bitPosition_ >> 3;
            const unsigned bitIndex = static_cast<unsigned>(bitPosition_ & 7u);
            const uint32_t bit = (static_cast<uint32_t>(data_[byteIndex]) >> bitIndex) & 1u;
            result |= bit << i;
            ++bitPosition_;
        }
        value = result;
        return true;
    }

    bool AlignToByte() {
        bitPosition_ = (bitPosition_ + 7u) & ~static_cast<size_t>(7u);
        return bitPosition_ <= bitSize_;
    }

private:
    const unsigned char* data_;
    size_t bitSize_;
    size_t bitPosition_;
};

class HuffmanTree {
public:
    bool Build(const uint8_t* lengths, size_t lengthCount, std::string& error) {
        counts_.fill(0);
        symbols_.clear();
        if (lengthCount == 0) {
            error = "空 Huffman 树";
            return false;
        }
        for (size_t i = 0; i < lengthCount; ++i) {
            if (lengths[i] > 15) {
                error = "Huffman 编码长度无效";
                return false;
            }
            ++counts_[lengths[i]];
        }
        if (counts_[0] == lengthCount) {
            return true;  // Empty tree; any attempt to decode from it fails.
        }

        int left = 1;
        for (int len = 1; len <= 15; ++len) {
            left = (left << 1) - static_cast<int>(counts_[len]);
            if (left < 0) {
                error = "Huffman 树过度订阅（编码长度无效）";
                return false;
            }
        }

        std::array<uint16_t, 16> offsets{};
        offsets[1] = 0;
        for (int len = 1; len < 15; ++len) {
            offsets[len + 1] = static_cast<uint16_t>(offsets[len] + counts_[len]);
        }
        symbols_.resize(lengthCount);
        for (size_t symbol = 0; symbol < lengthCount; ++symbol) {
            const uint8_t length = lengths[symbol];
            if (length != 0) symbols_[offsets[length]++] = static_cast<uint16_t>(symbol);
        }
        return true;
    }

    bool Decode(BitReader& reader, uint16_t& symbol) const {
        int code = 0;
        int first = 0;
        int index = 0;
        for (int len = 1; len <= 15; ++len) {
            uint32_t bit = 0;
            if (!reader.ReadBits(1, bit)) return false;
            code |= static_cast<int>(bit);
            const int count = counts_[len];
            if (code - first < count) {
                const size_t position = static_cast<size_t>(index + (code - first));
                if (position >= symbols_.size()) return false;
                symbol = symbols_[position];
                return true;
            }
            index += count;
            first = (first + count) << 1;
            code <<= 1;
        }
        return false;
    }

private:
    std::array<uint16_t, 16> counts_{};
    std::vector<uint16_t> symbols_;
};

constexpr std::array<uint16_t, 29> kLengthBase = {
    3,   4,   5,   6,   7,   8,   9,   10,  11,  13,  15,  17,  19,  23, 27,
    31,  35,  43,  51,  59,  67,  83,  99,  115, 131, 163, 195, 227, 258};
constexpr std::array<uint8_t, 29> kLengthExtra = {
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2,
    2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};
constexpr std::array<uint16_t, 30> kDistanceBase = {
    1,    2,    3,    4,    5,    7,     9,     13,    17,    25,
    33,   49,   65,   97,   129,  193,   257,   385,   513,   769,
    1025, 1537, 2049, 3073, 4097, 6145,  8193,  12289, 16385, 24577};
constexpr std::array<uint8_t, 30> kDistanceExtra = {
    0, 0, 0,  0,  1,  1,  2,  2,  3,  3,  4,  4,  5,  5,  6,
    6, 7, 7,  8,  8,  9,  9,  10, 10, 11, 11, 12, 12, 13, 13};

class Inflater {
public:
    Inflater(size_t expectedSize, std::string& output, std::string& error)
        : expected_(expectedSize), output_(output), error_(error) {
        output_.clear();
        output_.reserve(expectedSize);
    }

    bool Run(const unsigned char* data, size_t size) {
        BitReader reader(data, size);
        bool lastBlock = false;
        uint32_t blockCount = 0;
        while (!lastBlock) {
            uint32_t last = 0;
            uint32_t type = 0;
            if (!reader.ReadBits(1, last) || !reader.ReadBits(2, type)) {
                return Fail("压缩数据在最终块之前结束");
            }
            lastBlock = last != 0;
            bool ok = false;
            switch (type) {
                case 0:
                    ok = Stored(reader);
                    break;
                case 1:
                    ok = Fixed(reader);
                    break;
                case 2:
                    ok = Dynamic(reader);
                    break;
                default:
                    return Fail("保留了无效的 DEFLATE 块类型");
            }
            if (!ok) return false;
            if (++blockCount > 1000000u) return Fail("DEFLATE 块数量异常");
        }
        if (output_.size() != expected_) {
            return Fail("解压后大小与 ZIP 目录记录不一致");
        }
        return true;
    }

private:
    bool Fail(std::string message) {
        if (error_.empty()) error_ = std::move(message);
        return false;
    }

    bool AppendByte(uint8_t value) {
        if (output_.size() >= expected_) {
            return Fail("DEFLATE 数据超出 ZIP 目录记录的大小");
        }
        output_.push_back(static_cast<char>(value));
        return true;
    }

    bool CopyFromHistory(size_t distance, size_t length) {
        if (distance == 0 || distance > output_.size()) {
            return Fail("DEFLATE 回溯距离无效");
        }
        if (length > expected_ - output_.size()) {
            return Fail("DEFLATE 数据超出 ZIP 目录记录的大小");
        }
        for (size_t i = 0; i < length; ++i) {
            output_.push_back(output_[output_.size() - distance]);
        }
        return true;
    }

    bool Stored(BitReader& reader) {
        if (!reader.AlignToByte()) return Fail("压缩数据已结束");
        uint32_t length = 0;
        uint32_t complement = 0;
        if (!reader.ReadBits(16, length) || !reader.ReadBits(16, complement)) {
            return Fail("DEFLATE 存储块头不完整");
        }
        if ((length ^ 0xFFFFu) != complement) {
            return Fail("DEFLATE 存储块长度校验失败");
        }
        if (static_cast<size_t>(length) > expected_ - output_.size()) {
            return Fail("DEFLATE 数据超出 ZIP 目录记录的大小");
        }
        for (uint32_t i = 0; i < length; ++i) {
            uint32_t byte = 0;
            if (!reader.ReadBits(8, byte)) return Fail("DEFLATE 存储块数据不完整");
            output_.push_back(static_cast<char>(byte));
        }
        return true;
    }

    bool Fixed(BitReader& reader) {
        std::array<uint8_t, 288> literalLengths{};
        for (size_t i = 0; i <= 143; ++i) literalLengths[i] = 8;
        for (size_t i = 144; i <= 255; ++i) literalLengths[i] = 9;
        for (size_t i = 256; i <= 279; ++i) literalLengths[i] = 7;
        for (size_t i = 280; i <= 287; ++i) literalLengths[i] = 8;

        std::array<uint8_t, 32> distanceLengths{};
        distanceLengths.fill(5);

        HuffmanTree literalTree;
        HuffmanTree distanceTree;
        if (!literalTree.Build(literalLengths.data(), literalLengths.size(), error_)) return false;
        if (!distanceTree.Build(distanceLengths.data(), distanceLengths.size(), error_)) return false;
        return Codes(reader, literalTree, distanceTree);
    }

    bool Dynamic(BitReader& reader) {
        uint32_t hlit = 0;
        uint32_t hdist = 0;
        uint32_t hclen = 0;
        if (!reader.ReadBits(5, hlit) || !reader.ReadBits(5, hdist) ||
            !reader.ReadBits(4, hclen)) {
            return Fail("DEFLATE 动态块头不完整");
        }

        const size_t literalCount = static_cast<size_t>(hlit) + 257;
        const size_t distanceCount = static_cast<size_t>(hdist) + 1;
        const size_t codeLengthCount = static_cast<size_t>(hclen) + 4;
        if (literalCount + distanceCount > 320) {
            return Fail("DEFLATE 动态块尺寸无效");
        }

        static constexpr std::array<size_t, 19> kCodeLengthOrder = {
            16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
        std::array<uint8_t, 19> codeLengths{};
        for (size_t i = 0; i < codeLengthCount; ++i) {
            uint32_t value = 0;
            if (!reader.ReadBits(3, value)) return Fail("DEFLATE 编码长度表不完整");
            codeLengths[kCodeLengthOrder[i]] = static_cast<uint8_t>(value);
        }

        HuffmanTree codeLengthTree;
        if (!codeLengthTree.Build(codeLengths.data(), codeLengths.size(), error_)) return false;

        std::array<uint8_t, 320> lengths{};
        const size_t total = literalCount + distanceCount;
        size_t index = 0;
        while (index < total) {
            uint16_t symbol = 0;
            if (!codeLengthTree.Decode(reader, symbol)) {
                return Fail("DEFLATE 编码长度符号无效");
            }
            if (symbol < 16) {
                lengths[index++] = static_cast<uint8_t>(symbol);
            } else if (symbol == 16) {
                if (index == 0) return Fail("DEFLATE 重复编码长度缺少前值");
                uint32_t extra = 0;
                if (!reader.ReadBits(2, extra)) return Fail("DEFLATE 重复编码长度不完整");
                const size_t repeat = static_cast<size_t>(extra) + 3;
                if (repeat > total - index) return Fail("DEFLATE 重复编码长度过多");
                const uint8_t previous = lengths[index - 1];
                for (size_t i = 0; i < repeat; ++i) lengths[index++] = previous;
            } else if (symbol == 17) {
                uint32_t extra = 0;
                if (!reader.ReadBits(3, extra)) return Fail("DEFLATE 零长度重复不完整");
                const size_t repeat = static_cast<size_t>(extra) + 3;
                if (repeat > total - index) return Fail("DEFLATE 零长度重复过多");
                for (size_t i = 0; i < repeat; ++i) lengths[index++] = 0;
            } else if (symbol == 18) {
                uint32_t extra = 0;
                if (!reader.ReadBits(7, extra)) return Fail("DEFLATE 零长度重复不完整");
                const size_t repeat = static_cast<size_t>(extra) + 11;
                if (repeat > total - index) return Fail("DEFLATE 零长度重复过多");
                for (size_t i = 0; i < repeat; ++i) lengths[index++] = 0;
            } else {
                return Fail("DEFLATE 编码长度符号无效");
            }
        }

        if (lengths[256] == 0) return Fail("DEFLATE 数据缺少块结束码");

        HuffmanTree literalTree;
        HuffmanTree distanceTree;
        if (!literalTree.Build(lengths.data(), literalCount, error_)) return false;
        if (!distanceTree.Build(lengths.data() + literalCount, distanceCount, error_)) return false;
        return Codes(reader, literalTree, distanceTree);
    }

    bool Codes(BitReader& reader, const HuffmanTree& literalTree,
               const HuffmanTree& distanceTree) {
        for (;;) {
            uint16_t symbol = 0;
            if (!literalTree.Decode(reader, symbol)) {
                return Fail("DEFLATE 字面量/长度码无效或数据被截断");
            }
            if (symbol < 256) {
                if (!AppendByte(static_cast<uint8_t>(symbol))) return false;
                continue;
            }
            if (symbol == 256) return true;
            if (symbol > 285) return Fail("DEFLATE 长度符号无效");

            const size_t lengthIndex = static_cast<size_t>(symbol - 257);
            uint32_t lengthExtra = 0;
            if (!reader.ReadBits(kLengthExtra[lengthIndex], lengthExtra)) {
                return Fail("DEFLATE 长度附加位不完整");
            }
            const size_t length =
                static_cast<size_t>(kLengthBase[lengthIndex]) + static_cast<size_t>(lengthExtra);

            uint16_t distanceSymbol = 0;
            if (!distanceTree.Decode(reader, distanceSymbol)) {
                return Fail("DEFLATE 距离码无效或数据被截断");
            }
            if (distanceSymbol > 29) return Fail("DEFLATE 距离符号无效");
            uint32_t distanceExtra = 0;
            if (!reader.ReadBits(kDistanceExtra[distanceSymbol], distanceExtra)) {
                return Fail("DEFLATE 距离附加位不完整");
            }
            const size_t distance = static_cast<size_t>(kDistanceBase[distanceSymbol]) +
                                    static_cast<size_t>(distanceExtra);
            if (!CopyFromHistory(distance, length)) return false;
        }
    }

    size_t expected_;
    std::string& output_;
    std::string& error_;
};

// ---------------------------------------------------------------------------
// ZIP central-directory reader
// ---------------------------------------------------------------------------

struct ZipEntryInfo {
    uint16_t flags = 0;
    uint16_t method = 0;
    uint32_t crc = 0;
    uint64_t compressedSize = 0;
    uint64_t uncompressedSize = 0;
    uint64_t localHeaderOffset = 0;
};

std::string NormalizeEntryName(const std::string& name) {
    std::string normalized = name;
    for (char& ch : normalized) {
        if (ch == '\\') {
            ch = '/';
        } else if (ch >= 'A' && ch <= 'Z') {
            ch = static_cast<char>(ch - 'A' + 'a');
        }
    }
    return normalized;
}

bool ApplyZip64Extra(const unsigned char* extra, size_t extraSize, bool needUncompressed,
                     bool needCompressed, bool needOffset, uint64_t& uncompressed,
                     uint64_t& compressed, uint64_t& offset, std::string& error) {
    size_t position = 0;
    while (position + 4 <= extraSize) {
        const uint16_t id = ReadLe16(extra + position);
        const uint16_t size = ReadLe16(extra + position + 2);
        position += 4;
        if (static_cast<size_t>(size) > extraSize - position) {
            error = "ZIP 扩展字段长度无效";
            return false;
        }
        if (id == 0x0001u) {
            size_t field = position;
            const size_t end = position + static_cast<size_t>(size);
            if (needUncompressed) {
                if (end - field < 8) {
                    error = "ZIP64 字段缺少未压缩大小";
                    return false;
                }
                uncompressed = ReadLe64(extra + field);
                field += 8;
            }
            if (needCompressed) {
                if (end - field < 8) {
                    error = "ZIP64 字段缺少压缩大小";
                    return false;
                }
                compressed = ReadLe64(extra + field);
                field += 8;
            }
            if (needOffset) {
                if (end - field < 8) {
                    error = "ZIP64 字段缺少本地文件头偏移";
                    return false;
                }
                offset = ReadLe64(extra + field);
            }
            return true;
        }
        position += static_cast<size_t>(size);
    }
    error = "ZIP64 扩展字段缺失或损坏";
    return false;
}

bool FindZipEntry(const std::string& archive, std::string_view wantedName, ZipEntryInfo& entry,
                  std::string& error) {
    const size_t archiveSize = archive.size();
    if (archiveSize < 22) {
        error = "不是有效的 DOCX/ZIP 文件（文件过小）";
        return false;
    }

    // The EOCD record is at the end, but a ZIP comment may follow it. Scan
    // backwards through the maximum possible comment length.
    size_t eocdPosition = 0;
    bool foundEocd = false;
    const size_t lowest = archiveSize > 22 + 65535 ? archiveSize - 22 - 65535 : 0;
    for (size_t position = archiveSize - 22;; --position) {
        if (ReadLe32(reinterpret_cast<const unsigned char*>(archive.data()) + position) ==
            kZipEndSignature) {
            const unsigned char* eocd =
                reinterpret_cast<const unsigned char*>(archive.data()) + position;
            const uint16_t commentLength = ReadLe16(eocd + 20);
            if (static_cast<size_t>(commentLength) <= archiveSize - position - 22) {
                eocdPosition = position;
                foundEocd = true;
                break;
            }
        }
        if (position == lowest) break;
    }
    if (!foundEocd) {
        error = "不是有效的 DOCX/ZIP 文件（找不到中央目录结束记录）";
        return false;
    }

    const unsigned char* eocd =
        reinterpret_cast<const unsigned char*>(archive.data()) + eocdPosition;
    const uint16_t diskNumber = ReadLe16(eocd + 4);
    const uint16_t centralDisk = ReadLe16(eocd + 6);
    const uint16_t entriesOnDisk = ReadLe16(eocd + 8);
    const uint16_t entriesTotal = ReadLe16(eocd + 10);
    const uint32_t centralSize = ReadLe32(eocd + 12);
    const uint32_t centralOffset = ReadLe32(eocd + 16);

    if (diskNumber != 0 || centralDisk != 0) {
        error = "不支持分卷 ZIP 文件";
        return false;
    }
    if (entriesOnDisk != entriesTotal) {
        error = "ZIP 中央目录条目数量不一致";
        return false;
    }
    if (entriesTotal == 0xFFFFu || centralSize == 0xFFFFFFFFu ||
        centralOffset == 0xFFFFFFFFu) {
        error = "暂不支持 ZIP64 格式的 DOCX（文件过大）";
        return false;
    }
    if (static_cast<uint64_t>(centralOffset) > archiveSize ||
        static_cast<uint64_t>(centralSize) > archiveSize - centralOffset) {
        error = "ZIP 中央目录超出文件范围";
        return false;
    }

    const size_t centralEnd = static_cast<size_t>(centralOffset) + centralSize;
    size_t position = centralOffset;
    for (uint32_t i = 0; i < entriesTotal; ++i) {
        if (centralEnd - position < 46) {
            error = "ZIP 中央目录条目被截断";
            return false;
        }
        const unsigned char* record =
            reinterpret_cast<const unsigned char*>(archive.data()) + position;
        if (ReadLe32(record) != kZipCentralSignature) {
            error = "ZIP 中央目录签名无效";
            return false;
        }

        const uint16_t flags = ReadLe16(record + 8);
        const uint16_t method = ReadLe16(record + 10);
        const uint32_t crc = ReadLe32(record + 16);
        const uint32_t compressed32 = ReadLe32(record + 20);
        const uint32_t uncompressed32 = ReadLe32(record + 24);
        const uint16_t nameLength = ReadLe16(record + 28);
        const uint16_t extraLength = ReadLe16(record + 30);
        const uint16_t commentLength = ReadLe16(record + 32);
        const uint32_t localOffset32 = ReadLe32(record + 42);

        const uint64_t recordSize = 46ULL + static_cast<uint64_t>(nameLength) + extraLength +
                                    commentLength;
        if (recordSize > centralEnd - position) {
            error = "ZIP 中央目录条目被截断";
            return false;
        }

        const std::string name(reinterpret_cast<const char*>(record + 46), nameLength);
        if (NormalizeEntryName(name) != wantedName) {
            position += static_cast<size_t>(recordSize);
            continue;
        }

        uint64_t compressed = compressed32;
        uint64_t uncompressed = uncompressed32;
        uint64_t localOffset = localOffset32;
        if (compressed32 == 0xFFFFFFFFu || uncompressed32 == 0xFFFFFFFFu ||
            localOffset32 == 0xFFFFFFFFu) {
            const unsigned char* extra = record + 46 + nameLength;
            if (!ApplyZip64Extra(extra, extraLength, uncompressed32 == 0xFFFFFFFFu,
                                 compressed32 == 0xFFFFFFFFu,
                                 localOffset32 == 0xFFFFFFFFu, uncompressed, compressed,
                                 localOffset, error)) {
                return false;
            }
        }

        entry.flags = flags;
        entry.method = method;
        entry.crc = crc;
        entry.compressedSize = compressed;
        entry.uncompressedSize = uncompressed;
        entry.localHeaderOffset = localOffset;
        return true;
    }

    error = "DOCX 包中缺少 word/document.xml";
    return false;
}

bool LocateEntryData(const std::string& archive, const ZipEntryInfo& entry, size_t& dataOffset,
                     std::string& error) {
    const size_t archiveSize = archive.size();
    if (entry.localHeaderOffset > archiveSize ||
        archiveSize - static_cast<size_t>(entry.localHeaderOffset) < 30) {
        error = "ZIP 本地文件头超出文件范围";
        return false;
    }

    const unsigned char* local =
        reinterpret_cast<const unsigned char*>(archive.data()) +
        static_cast<size_t>(entry.localHeaderOffset);
    if (ReadLe32(local) != kZipLocalSignature) {
        error = "ZIP 本地文件头签名无效";
        return false;
    }
    const uint16_t nameLength = ReadLe16(local + 26);
    const uint16_t extraLength = ReadLe16(local + 28);
    const uint64_t offset = entry.localHeaderOffset + 30ULL + nameLength + extraLength;
    if (offset > archiveSize || entry.compressedSize > archiveSize - offset) {
        error = "ZIP 条目数据超出文件范围";
        return false;
    }
    dataOffset = static_cast<size_t>(offset);
    return true;
}

bool ReadDocumentXml(const std::string& archive, std::string& xml, std::string& error) {
    ZipEntryInfo entry;
    if (!FindZipEntry(archive, "word/document.xml", entry, error)) return false;
    if ((entry.flags & 0x0001u) != 0) {
        error = "DOCX 中的 word/document.xml 已加密，无法读取";
        return false;
    }
    if (entry.uncompressedSize > kMaxDocumentXmlSize) {
        error = "word/document.xml 过大（" + Number(entry.uncompressedSize) + " 字节），上限为 " +
                Number(kMaxDocumentXmlSize / (1024ULL * 1024ULL)) + " MB";
        return false;
    }

    size_t dataOffset = 0;
    if (!LocateEntryData(archive, entry, dataOffset, error)) return false;
    const unsigned char* data =
        reinterpret_cast<const unsigned char*>(archive.data()) + dataOffset;

    if (entry.method == 0) {
        if (entry.compressedSize != entry.uncompressedSize) {
            error = "ZIP 存储条目的压缩大小与原始大小不一致";
            return false;
        }
        xml.assign(reinterpret_cast<const char*>(data),
                   static_cast<size_t>(entry.compressedSize));
    } else if (entry.method == 8) {
        std::string inflateError;
        if (!Inflater(static_cast<size_t>(entry.uncompressedSize), xml, inflateError)
                 .Run(data, static_cast<size_t>(entry.compressedSize))) {
            error = "无法解压 word/document.xml：" + inflateError;
            return false;
        }
    } else {
        error = "DOCX 使用了不支持的 ZIP 压缩方式（" + Number(entry.method) +
                "），仅支持存储（0）和 DEFLATE（8）";
        return false;
    }

    const uint32_t actualCrc = Crc32(reinterpret_cast<const unsigned char*>(xml.data()), xml.size());
    if (actualCrc != entry.crc) {
        error = "DOCX 校验失败（CRC-32 不匹配），文件可能已损坏";
        xml.clear();
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// WordprocessingML -> plain text
// ---------------------------------------------------------------------------

bool IsAsciiSpace(char ch) {
    return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
}

size_t FindTagEnd(const std::string& xml, size_t start) {
    char quote = 0;
    for (size_t i = start + 1; i < xml.size(); ++i) {
        const char ch = xml[i];
        if (quote != 0) {
            if (ch == quote) quote = 0;
        } else if (ch == '"' || ch == '\'') {
            quote = ch;
        } else if (ch == '>') {
            return i;
        }
    }
    return std::string::npos;
}

int HexDigit(char ch) {
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    return -1;
}

void AppendDecodedXmlText(std::string_view raw, std::string& out) {
    for (size_t i = 0; i < raw.size();) {
        if (raw[i] != '&') {
            out.push_back(raw[i]);
            ++i;
            continue;
        }
        const size_t semicolon = raw.find(';', i + 1);
        if (semicolon == std::string_view::npos || semicolon - i > 32) {
            out.push_back('&');
            ++i;
            continue;
        }
        const std::string_view entity = raw.substr(i + 1, semicolon - i - 1);
        if (entity == "amp") {
            out.push_back('&');
        } else if (entity == "lt") {
            out.push_back('<');
        } else if (entity == "gt") {
            out.push_back('>');
        } else if (entity == "quot") {
            out.push_back('"');
        } else if (entity == "apos") {
            out.push_back('\'');
        } else if (entity.size() >= 2 && entity[0] == '#') {
            bool valid = true;
            uint32_t value = 0;
            if (entity[1] == 'x' || entity[1] == 'X') {
                if (entity.size() == 2) {
                    valid = false;
                } else {
                    for (size_t k = 2; k < entity.size() && valid; ++k) {
                        const int digit = HexDigit(entity[k]);
                        if (digit < 0 || value > 0x10FFFFu / 16u) {
                            valid = false;
                        } else {
                            value = value * 16u + static_cast<uint32_t>(digit);
                        }
                    }
                }
            } else {
                if (entity.size() == 1) {
                    valid = false;
                } else {
                    for (size_t k = 1; k < entity.size() && valid; ++k) {
                        const char ch = entity[k];
                        if (ch < '0' || ch > '9') {
                            valid = false;
                        } else {
                            const uint32_t digit = static_cast<uint32_t>(ch - '0');
                            if (value > (0x10FFFFu - digit) / 10u) {
                                valid = false;
                            } else {
                                value = value * 10u + digit;
                            }
                        }
                    }
                }
            }

            if (!valid || value == 0 || value > 0x10FFFFu ||
                (value >= 0xD800u && value <= 0xDFFFu)) {
                out.append("\xEF\xBF\xBD");  // U+FFFD REPLACEMENT CHARACTER
            } else {
                AppendUtf8(out, value);
            }
        } else {
            // Unknown entity: preserve it verbatim rather than dropping data.
            out.append(raw.substr(i, semicolon - i + 1));
        }
        i = semicolon + 1;
    }
}

bool NormalizeXmlEncoding(std::string& xml, std::string& error) {
    if (xml.size() >= 2 && static_cast<unsigned char>(xml[0]) == 0xFFu &&
        static_cast<unsigned char>(xml[1]) == 0xFEu) {
        std::string converted;
        std::string detail;
        if (!Utf16ToUtf8(std::string_view(xml).substr(2), true, converted, detail)) {
            error = "word/document.xml 的 UTF-16LE 编码无效：" + detail;
            return false;
        }
        xml = std::move(converted);
    } else if (xml.size() >= 2 && static_cast<unsigned char>(xml[0]) == 0xFEu &&
               static_cast<unsigned char>(xml[1]) == 0xFFu) {
        std::string converted;
        std::string detail;
        if (!Utf16ToUtf8(std::string_view(xml).substr(2), false, converted, detail)) {
            error = "word/document.xml 的 UTF-16BE 编码无效：" + detail;
            return false;
        }
        xml = std::move(converted);
    } else if (xml.size() >= 3 && static_cast<unsigned char>(xml[0]) == 0xEFu &&
               static_cast<unsigned char>(xml[1]) == 0xBBu &&
               static_cast<unsigned char>(xml[2]) == 0xBFu) {
        xml.erase(0, 3);
    }

    size_t badOffset = 0;
    if (!ValidateUtf8(xml, badOffset)) {
        error = "word/document.xml 不是有效的 UTF-8（字节偏移 " + Number(badOffset) + "）";
        return false;
    }
    return true;
}

bool ExtractTextFromDocumentXml(std::string xml, std::string& text, std::string& error) {
    if (!NormalizeXmlEncoding(xml, error)) return false;

    std::string output;
    bool inText = false;
    bool sawDocument = false;
    int paragraphPropertiesDepth = 0;
    int fallbackDepth = 0;
    size_t paragraphCount = 0;
    size_t position = 0;

    while (position < xml.size()) {
        if (xml[position] != '<') {
            const size_t end = xml.find('<', position);
            const size_t chunkEnd = end == std::string::npos ? xml.size() : end;
            if (inText && fallbackDepth == 0) {
                AppendDecodedXmlText(std::string_view(xml).substr(position, chunkEnd - position),
                                     output);
            }
            position = chunkEnd;
            continue;
        }

        if (xml.compare(position, 4, "<!--") == 0) {
            const size_t end = xml.find("-->", position + 4);
            if (end == std::string::npos) {
                error = "word/document.xml 中的 XML 注释未结束";
                return false;
            }
            position = end + 3;
            continue;
        }
        if (xml.compare(position, 9, "<![CDATA[") == 0) {
            const size_t end = xml.find("]]>", position + 9);
            if (end == std::string::npos) {
                error = "word/document.xml 中的 CDATA 段未结束";
                return false;
            }
            if (inText && fallbackDepth == 0) {
                output.append(xml, position + 9, end - (position + 9));
            }
            position = end + 3;
            continue;
        }
        if (xml.compare(position, 2, "<?") == 0) {
            const size_t end = xml.find("?>", position + 2);
            if (end == std::string::npos) {
                error = "word/document.xml 中的处理指令未结束";
                return false;
            }
            position = end + 2;
            continue;
        }
        if (xml.compare(position, 2, "<!") == 0) {
            const size_t end = xml.find('>', position + 2);
            if (end == std::string::npos) {
                error = "word/document.xml 中的 XML 声明未结束";
                return false;
            }
            position = end + 1;
            continue;
        }

        const size_t tagEnd = FindTagEnd(xml, position);
        if (tagEnd == std::string::npos) {
            error = "word/document.xml 中的 XML 标签未结束";
            return false;
        }

        size_t cursor = position + 1;
        bool closing = false;
        if (cursor < tagEnd && xml[cursor] == '/') {
            closing = true;
            ++cursor;
        }
        while (cursor < tagEnd && IsAsciiSpace(xml[cursor])) ++cursor;
        const size_t nameStart = cursor;
        while (cursor < tagEnd && !IsAsciiSpace(xml[cursor]) && xml[cursor] != '/' &&
               xml[cursor] != '>') {
            ++cursor;
        }
        const std::string_view name(xml.data() + nameStart, cursor - nameStart);

        size_t last = tagEnd;
        while (last > position && IsAsciiSpace(xml[last - 1])) --last;
        const bool selfClosing = last > position && xml[last - 1] == '/';

        if (!closing) {
            if (name == "w:document") sawDocument = true;
            if (name == "w:pPr" && !selfClosing) ++paragraphPropertiesDepth;
            if (name == "mc:Fallback" && !selfClosing) ++fallbackDepth;

            if (fallbackDepth == 0) {
                if (name == "w:p") {
                    if (paragraphCount > 0) output.push_back('\n');
                    ++paragraphCount;
                } else if (name == "w:t" && !selfClosing) {
                    inText = true;
                } else if (name == "w:tab" || name == "w:ptab") {
                    if (paragraphPropertiesDepth == 0) output.push_back('\t');
                } else if (name == "w:br" || name == "w:cr") {
                    if (paragraphPropertiesDepth == 0) output.push_back('\n');
                } else if (name == "w:noBreakHyphen") {
                    output.push_back('-');
                }
            }
        } else {
            if (name == "w:pPr") {
                if (paragraphPropertiesDepth > 0) --paragraphPropertiesDepth;
            } else if (name == "mc:Fallback") {
                if (fallbackDepth > 0) --fallbackDepth;
            } else if (name == "w:t") {
                inText = false;
            }
        }

        position = tagEnd + 1;
    }

    if (!sawDocument) {
        error = "word/document.xml 不是有效的 WordprocessingML 文档（缺少 <w:document>）";
        return false;
    }
    if (inText) {
        error = "word/document.xml 中的 <w:t> 未闭合";
        return false;
    }

    NormalizeNewlinesInPlace(output);
    size_t badOffset = 0;
    if (!ValidateUtf8(output, badOffset)) {
        error = "从 DOCX 提取的文本不是有效的 UTF-8（字节偏移 " + Number(badOffset) + "）";
        return false;
    }

    text = std::move(output);
    return true;
}

// ---------------------------------------------------------------------------
// Public entry points
// ---------------------------------------------------------------------------

std::string UnsupportedTypeError(const std::string& extension) {
    if (extension.empty()) {
        return "不支持的文件类型（无扩展名）；支持 .txt、.md、.docx";
    }
    return "不支持的文件类型（" + extension + "）；支持 .txt、.md、.docx";
}

}  // namespace

TextFileResult ExtractTextFile(const std::filesystem::path& path) {
    TextFileResult result;
    try {
        const std::string extension = ExtensionLower(path);

        if (extension == ".pdf") {
            result.error =
                WithPath(path, "暂不支持 PDF 文本提取（当前版本没有内置 PDF 解析器）；请先用 Word "
                               "或其他工具转换为 .docx 或 .txt 后重试");
            return result;
        }
        if (extension == ".doc") {
            result.error =
                WithPath(path, "暂不支持旧版 .doc 二进制 Word 文档；请在 Word/WPS 中另存为 .docx 后重试");
            return result;
        }
        if (extension != ".txt" && extension != ".md" && extension != ".docx") {
            result.error = WithPath(path, UnsupportedTypeError(extension));
            return result;
        }

        std::string bytes;
        std::string detail;
        if (!ReadWholeFile(path, bytes, detail)) {
            result.error = WithPath(path, detail);
            return result;
        }

        std::string text;
        if (extension == ".txt" || extension == ".md") {
            if (!DecodePlainText(bytes, text, detail)) {
                result.error = WithPath(path, detail);
                return result;
            }
        } else {
            std::string xml;
            if (!ReadDocumentXml(bytes, xml, detail)) {
                result.error = WithPath(path, detail);
                return result;
            }
            auto countOccurrences = [](const std::string& source, const std::string& needle) {
                size_t count = 0;
                size_t position = 0;
                while ((position = source.find(needle, position)) != std::string::npos) {
                    ++count;
                    position += needle.size();
                }
                return count;
            };
            result.textRuns = static_cast<int>(countOccurrences(xml, "<w:t"));
            result.mathTypeObjects = static_cast<int>(countOccurrences(xml, "Equation.DSMT4"));
            result.drawingObjects = static_cast<int>(countOccurrences(xml, "<w:drawing"));
            if (result.mathTypeObjects > 0) {
                result.warning =
                    "该 DOCX 包含 " + Number(static_cast<uint64_t>(result.mathTypeObjects)) +
                    " 个 MathType/WMF 公式对象，公式不在文字层中";
            }
            if (!ExtractTextFromDocumentXml(std::move(xml), text, detail)) {
                result.error = WithPath(path, detail);
                return result;
            }
        }

        result.text = std::move(text);
        result.ok = true;
    } catch (const std::exception& exception) {
        result.ok = false;
        result.text.clear();
        result.error = WithPath(path, std::string("读取文件时发生意外错误：") + exception.what());
    } catch (...) {
        result.ok = false;
        result.text.clear();
        result.error = WithPath(path, "读取文件时发生未知错误");
    }
    return result;
}

bool IsSupportedTextDocument(const std::filesystem::path& path) {
    try {
        const std::string extension = ExtensionLower(path);
        return extension == ".txt" || extension == ".md" || extension == ".docx";
    } catch (...) {
        return false;
    }
}
