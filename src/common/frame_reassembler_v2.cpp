//==============================================================//
/*
 * frame_reassembler_v2.cpp
 *
 *  Created on: 2026-01-10
 *
 */ 
//==============================================================//


#include "debug/hv_debug.hpp"
#include "common/frame_reassembler_v2.hpp"

#include <cstring>
#include <algorithm>

FrameReassemblerV2::FrameReassemblerV2(size_t max_frame_size,
                                       size_t payload_stride)
    : max_frame_size_(max_frame_size),
      payload_stride_(payload_stride),
      current_frame_id_(0),
      expected_packet_count_(0),
      received_packets_count_(0),
      corrupted_detected_(false)
{
    frame_buffer_.resize(max_frame_size_);
}

void FrameReassemblerV2::startNewFrame(uint32_t frame_id,
                                       uint16_t packet_count)
{
    current_frame_id_       = frame_id;
    expected_packet_count_  = packet_count;
    received_packets_count_ = 0;
    corrupted_detected_     = false;
    frame_size_             = 0;

    packet_received_.assign(packet_count, false);
    packet_corrupted_.assign(packet_count, false);
    
}


/*-------------------------------------------*/
/* Packet reception                          */
/*-------------------------------------------*/
void FrameReassemblerV2::pushPacket(const UdpPacketHeader& hdr,
                                    const uint8_t* payload,
                                    bool gap_detected)
{
    uint16_t pid = hdr.packet_id;

    if (pid >= expected_packet_count_)
    {
        return;
    }

    if (packet_received_[pid])
    {
        // duplicate packet
        HV_LOGW(hv::debug::Module::FRAME, "[DUP ] frame=%u pid=%u", current_frame_id_, pid);
        return;
    }

    //Frame buffer size check
    size_t offset = static_cast<size_t>(pid) * payload_stride_;

    if (offset + hdr.payload_size > max_frame_size_) 
    {   
	    HV_LOGW(hv::debug::Module::FRAME, "[SIZE ] frame=%u pid=%u size=%u", current_frame_id_, pid, offset + hdr.payload_size);
        return;
    }

    
    //Correct data write (order does not matter)
    std::memcpy(frame_buffer_.data() + offset,
                payload,
                hdr.payload_size);

    packet_received_[pid] = true;
    received_packets_count_++;

    //size update
    size_t end = offset + hdr.payload_size;
    frame_size_ = std::max(frame_size_, end);
}

bool FrameReassemblerV2::hasAnyPacket() const
{
    return (received_packets_count_ > 0);
}

bool FrameReassemblerV2::isFrameComplete() const
{
    return ((received_packets_count_ == expected_packet_count_) 
            && (!corrupted_detected_));
}

bool FrameReassemblerV2::hasGap() const
{
    return (received_packets_count_ < expected_packet_count_);
}

bool FrameReassemblerV2::hasCorruption() const
{
    return corrupted_detected_;
}

bool FrameReassemblerV2::hasPacket(uint16_t packet_id) const
{
    if (packet_id >= expected_packet_count_)
        return false;

    return packet_received_[packet_id];
}


bool FrameReassemblerV2::hasPartialFrame() const
{
    return received_packets_count_ > 0;
}

const uint8_t* FrameReassemblerV2::getFrameData() const
{
    return frame_buffer_.data();
}

size_t FrameReassemblerV2::getFrameSize() const
{
    return frame_size_;
}


void FrameReassemblerV2::reset()
{
    expected_packet_count_ = 0;
    received_packets_count_ = 0;
    packet_received_.clear();
    corrupted_detected_ = false;
}


FrameResult FrameReassemblerV2::makeResult(FrameState final_state) const
{
    FrameResult r{};
    r.frame_id = current_frame_id_;
    r.state    = final_state;

    r.expected_packets = expected_packet_count_;
    r.received_packets = received_packets_count_;

    r.complete  = isFrameComplete();
    r.corrupted = hasGap() || hasCorruption();

    r.frame_size = frame_size_;
    r.frame_data = frame_buffer_.data();

    return r;
}

