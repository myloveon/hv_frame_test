#pragma once
#include <cstdint>

namespace protocol {

// UDP payload 최대 크기 (MTU 고려)
constexpr uint32_t MAX_UDP_PAYLOAD = 1400;

// Frame magic (CCSDS 121.0-B-2 stub 식별용)
constexpr uint32_t FRAME_MAGIC = 0xCC1210B2;

// Protocol version (확장 대비)
constexpr uint16_t PROTOCOL_VERSION = 1;

} // namespace protocol
