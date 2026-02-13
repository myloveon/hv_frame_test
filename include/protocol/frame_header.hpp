#pragma once
#include <cstdint>
#include "protocol_constants.hpp"

#pragma pack(push, 1)
struct FrameHeader {
    uint32_t magic;          // protocol::FRAME_MAGIC
    uint16_t version;        // protocol::PROTOCOL_VERSION
    uint16_t width;          // image width
    uint16_t height;         // image height
    uint8_t  bitdepth;       // RAW bit depth (10/12/16)
    uint8_t  reserved;       // alignment / future use
    uint32_t frame_size;     // payload size (after header)
};	//Frame unit
#pragma pack(pop)


