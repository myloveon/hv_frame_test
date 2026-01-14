#pragma once

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <chrono>
#include "protocol/udp_packet.hpp"

class FrameReassembler {
public:
    FrameReassembler();

    // Push one UDP packet
    void pushPacket(const UdpPacketHeader& hdr,
                    const uint8_t* payload);

    // Frame completion determination
    bool frameComplete() const;

    // Return completed frame (internal reset after call)
    std::vector<uint8_t> popFrame();

    // Forced reset (timeout, error)
    void reset();

private:
    void startNewFrame(uint32_t frame_id, uint16_t packet_count);
    bool isTimeout() const;

private:
    uint32_t current_frame_id_;
    uint16_t expected_packet_count_;

    std::unordered_map<uint16_t, std::vector<uint8_t>> packets_;
    uint16_t received_packet_count_;

    std::chrono::steady_clock::time_point frame_start_time_;

    static constexpr int FRAME_TIMEOUT_MS = 5000;
};
