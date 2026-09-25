#pragma once
#include <cstdint>
#include <cstring>
#include <string>

// Minimal protobuf decoder for foxglove.RawImage and foxglove.CompressedImage
namespace proto {

inline bool readVarint(const uint8_t* buf, size_t len, size_t& pos, uint64_t& val) {
    val = 0;
    int shift = 0;
    while (pos < len) {
        uint8_t b = buf[pos++];
        val |= (uint64_t)(b & 0x7F) << shift;
        if (!(b & 0x80)) return true;
        shift += 7;
        if (shift > 63) return false;
    }
    return false;
}

inline bool skipField(const uint8_t* buf, size_t len, size_t& pos, uint32_t wireType) {
    if (wireType == 0) {
        while (pos < len && (buf[pos++] & 0x80)) {}
        return pos <= len;
    } else if (wireType == 1) {
        pos += 8;
        return pos <= len;
    } else if (wireType == 2) {
        uint64_t n;
        if (!readVarint(buf, len, pos, n)) return false;
        pos += (size_t)n;
        return pos <= len;
    } else if (wireType == 5) {
        pos += 4;
        return pos <= len;
    }
    return false;
}

struct RawImage {
    uint32_t width = 0;
    uint32_t height = 0;
    std::string encoding;
    uint32_t step = 0;
    const uint8_t* data = nullptr;
    size_t dataLen = 0;
};

// foxglove.RawImage proto fields:
//   1: timestamp (message), 2: frame_id (string),
//   3: width (uint32), 4: height (uint32),
//   5: encoding (string), 6: step (uint32), 7: data (bytes)
inline bool parseRawImage(const uint8_t* buf, size_t len, RawImage& img) {
    size_t pos = 0;
    while (pos < len) {
        uint64_t tag;
        if (!readVarint(buf, len, pos, tag)) break;
        uint32_t field = (uint32_t)(tag >> 3);
        uint32_t wire  = (uint32_t)(tag & 0x7);

        if (wire == 0) {
            uint64_t v;
            if (!readVarint(buf, len, pos, v)) return false;
            if (field == 3) img.width  = (uint32_t)v;
            else if (field == 4) img.height = (uint32_t)v;
            else if (field == 6) img.step   = (uint32_t)v;
        } else if (wire == 2) {
            uint64_t n;
            if (!readVarint(buf, len, pos, n)) return false;
            if (pos + n > len) return false;
            if (field == 5) img.encoding = std::string((const char*)buf + pos, (size_t)n);
            else if (field == 7) { img.data = buf + pos; img.dataLen = (size_t)n; }
            pos += (size_t)n;
        } else {
            if (!skipField(buf, len, pos, wire)) break;
        }
    }
    return img.width > 0 && img.height > 0 && img.data != nullptr;
}

struct CompressedImage {
    const uint8_t* data = nullptr;
    size_t dataLen = 0;
    std::string format;
};

// foxglove.CompressedImage proto fields:
//   1: timestamp (message), 2: frame_id (string), 3: data (bytes), 4: format (string)
inline bool parseCompressedImage(const uint8_t* buf, size_t len, CompressedImage& img) {
    size_t pos = 0;
    while (pos < len) {
        uint64_t tag;
        if (!readVarint(buf, len, pos, tag)) break;
        uint32_t field = (uint32_t)(tag >> 3);
        uint32_t wire  = (uint32_t)(tag & 0x7);

        if (wire == 2) {
            uint64_t n;
            if (!readVarint(buf, len, pos, n)) return false;
            if (pos + n > len) return false;
            if (field == 3) { img.data = buf + pos; img.dataLen = (size_t)n; }
            else if (field == 4) img.format = std::string((const char*)buf + pos, (size_t)n);
            pos += (size_t)n;
        } else {
            if (!skipField(buf, len, pos, wire)) break;
        }
    }
    return img.data != nullptr;
}

} // namespace proto
