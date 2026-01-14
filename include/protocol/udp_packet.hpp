#pragma once
#include <cstdint>

#pragma pack(push, 1)
struct UdpPacketHeader {
    uint32_t frame_id;       // 프레임 식별자 (증가)
    uint16_t packet_id;      // 현재 패킷 번호
    uint16_t packet_count;   // 전체 패킷 수
    uint32_t payload_size;   // 이 패킷의 payload 크기
};
#pragma pack(pop)
