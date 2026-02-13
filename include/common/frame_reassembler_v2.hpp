#pragma once

#include <cstdint>
#include <vector>
#include <memory>
#include <span>
#include "protocol/udp_packet.hpp"
#include "common/frame_result.hpp"

struct UdpPacketHeader;

class FrameReassemblerV2
{
public:
    FrameReassemblerV2(size_t max_frame_size,
                       size_t payload_stride);

    // Frame start (Call when frame_id changes)
    void startNewFrame(uint32_t frame_id,
                       uint16_t packet_count);

    // Packet reception
    void pushPacket(const UdpPacketHeader& hdr,
                    const uint8_t* payload,
                    bool gap_detected);

    // Frame status
    bool frameComplete() const;
    bool hasPartialFrame() const;

    // Frame termination / Reset
    void reset();

    // Statistics
    uint16_t expectedPackets() const { return expected_packet_count_; }
    uint16_t receivedPackets() const { return received_packets_count_; }
    uint32_t frameId() const { return current_frame_id_; }

    // Additional status
    bool hasAnyPacket() const;
    bool isFrameComplete() const;     // Based on bitmap
    bool hasGap() const;              // The presence of dropped packets
    bool hasCorruption() const;       // CRC error inclusion status
    
    bool hasPacket(uint16_t packet_id) const;

    // Results-based approach
    const uint8_t* getFrameData() const;
    size_t getFrameSize() const;

    FrameResult makeResult(FrameState final_state) const;

private:
    // Fixed frame buffer
    size_t frame_size_ = 0;         // Actual recorded frame size
    size_t max_frame_size_;
    size_t payload_stride_;

    // Frame meta
    uint32_t current_frame_id_ = 0;
    uint16_t expected_packet_count_ = 0;
    uint16_t received_packets_count_ = 0;

    // Packet reception status
    std::vector<uint8_t> frame_buffer_;
    std::vector<bool> packet_received_;
    std::vector<bool> packet_corrupted_;

    bool corrupted_detected_ = false;
};
