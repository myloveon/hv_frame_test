#include "common/frame_reassembler.hpp"
#include <cstring>

FrameReassembler::FrameReassembler()
{
    reset();
}

void FrameReassembler::reset()
{
    current_frame_id_ = 0xFFFFFFFF;
    expected_packet_count_ = 0;
    received_packet_count_ = 0;
    packets_.clear();
}

void FrameReassembler::startNewFrame(uint32_t frame_id,
                                     uint16_t packet_count)
{
    reset();
    current_frame_id_ = frame_id;
    expected_packet_count_ = packet_count;
    frame_start_time_ = std::chrono::steady_clock::now();
}

bool FrameReassembler::isTimeout() const
{
    auto now = std::chrono::steady_clock::now();
    auto elapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            now - frame_start_time_).count();
    return elapsed > FRAME_TIMEOUT_MS;
}

void FrameReassembler::pushPacket(const UdpPacketHeader& hdr,
                                  const uint8_t* payload)
{
    // New frame start
    if (hdr.frame_id != current_frame_id_) {
        startNewFrame(hdr.frame_id, hdr.packet_count);
    }

    // frame timeout ¡æ discard
    if (isTimeout()) {
        reset();
        return;
    }

    // Ignore duplicate packets
    if (packets_.count(hdr.packet_id)) {
        return;
    }

    // payload storage
    packets_[hdr.packet_id] =
        std::vector<uint8_t>(payload, payload + hdr.payload_size);

    received_packet_count_++;
}

bool FrameReassembler::frameComplete() const
{
    return (expected_packet_count_ > 0 &&
            received_packet_count_ == expected_packet_count_);
}

std::vector<uint8_t> FrameReassembler::popFrame()
{
    std::vector<uint8_t> frame;

    if (!frameComplete())
        return frame;

    // Combine in order by packet ID
    for (uint16_t i = 0; i < expected_packet_count_; i++) {
        auto it = packets_.find(i);
        if (it == packets_.end()) {
            // In principle, you shouldn't be coming here.
            reset();
            return {};
        }
		
        frame.insert(frame.end(),
                     it->second.begin(),
                     it->second.end());
    }

    reset();
    return frame;
}
