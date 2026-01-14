#pragma once
#include <cstdint>

namespace protocol {

// Maximum UDP payload size (considering MTU)
constexpr uint32_t MAX_UDP_PAYLOAD = 1400;

// Frame magic (CCSDS 121.0-B-2 stub example)
constexpr uint32_t FRAME_MAGIC = 0xCC1210B2;

// Protocol version
constexpr uint16_t PROTOCOL_VERSION = 1;

} // namespace protocol
