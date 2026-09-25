#pragma once
#include <cstdint>
#include <cstring>
#include <string>

// Decode sensor_msgs/Image and sensor_msgs/CompressedImage from ROS1 wire format
namespace ros1 {

inline bool readU32(const uint8_t* buf, size_t len, size_t& pos, uint32_t& val) {
    if (pos + 4 > len) return false;
    memcpy(&val, buf + pos, 4);
    pos += 4;
    return true;
}

inline bool readStr(const uint8_t* buf, size_t len, size_t& pos, std::string& out) {
    uint32_t n;
    if (!readU32(buf, len, pos, n)) return false;
    if (pos + n > len) return false;
    out.assign((const char*)buf + pos, n);
    pos += n;
    return true;
}

struct Image {
    uint32_t height = 0;
    uint32_t width  = 0;
    std::string encoding;
    uint8_t is_bigendian = 0;
    uint32_t step = 0;
    const uint8_t* data = nullptr;
    size_t dataLen = 0;
};

// ROS1 sensor_msgs/Image layout:
//   Header { uint32 seq, time stamp(8), string frame_id }
//   uint32 height, uint32 width, string encoding
//   uint8 is_bigendian, uint32 step
//   uint8[] data  (prefixed with uint32 length)
inline bool parseImage(const uint8_t* buf, size_t len, Image& img) {
    size_t pos = 0;
    // skip seq (4)
    pos += 4;
    // skip stamp (8)
    pos += 8;
    if (pos > len) return false;
    // skip frame_id
    std::string dummy;
    if (!readStr(buf, len, pos, dummy)) return false;

    if (!readU32(buf, len, pos, img.height))   return false;
    if (!readU32(buf, len, pos, img.width))    return false;
    if (!readStr(buf, len, pos, img.encoding)) return false;
    if (pos >= len) return false;
    img.is_bigendian = buf[pos++];
    if (!readU32(buf, len, pos, img.step)) return false;

    uint32_t dataLen;
    if (!readU32(buf, len, pos, dataLen)) return false;
    if (pos + dataLen > len) return false;
    img.data    = buf + pos;
    img.dataLen = dataLen;
    return img.width > 0 && img.height > 0;
}

struct CompressedImage {
    std::string format;
    const uint8_t* data = nullptr;
    size_t dataLen = 0;
};

// ROS1 sensor_msgs/CompressedImage layout:
//   Header, string format, uint8[] data
inline bool parseCompressedImage(const uint8_t* buf, size_t len, CompressedImage& img) {
    size_t pos = 0;
    pos += 4 + 8; // seq + stamp
    if (pos > len) return false;
    std::string dummy;
    if (!readStr(buf, len, pos, dummy)) return false;  // frame_id
    if (!readStr(buf, len, pos, img.format)) return false;
    uint32_t dataLen;
    if (!readU32(buf, len, pos, dataLen)) return false;
    if (pos + dataLen > len) return false;
    img.data    = buf + pos;
    img.dataLen = dataLen;
    return img.data != nullptr;
}

} // namespace ros1
