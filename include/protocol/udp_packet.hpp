#pragma once
#include <cstdint>

#pragma pack(push, 1)
struct UdpPacketHeader {
    uint32_t frame_id;       // Frame identifier (increment)
    uint16_t packet_id;      // Current packet number
    uint16_t packet_count;   // Total number of packets
    uint32_t payload_size;   // the payload size of this packet
};

struct RxPacket {
    UdpPacketHeader hdr;
    uint8_t payload[1500];   // PAYLOAD_STRIDE
    bool gap_before = false;
};


#pragma pack(pop)
