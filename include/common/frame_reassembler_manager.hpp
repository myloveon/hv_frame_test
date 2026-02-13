/*---------------------------------------------------------------------------*/
/*                                                                           */
/*                    frame_reassembler_manager.hpp                          */
/*                                                                           */
/*  Frame Reassembler Manager for UDP Receiver                               */
/*                                                                           */
/*---------------------------------------------------------------------------*/


#pragma once

#include <map>
#include <cstdint>
#include <chrono>
#include <functional>

#include "common/frame_result.hpp"
#include "common/frame_reassembler_v2.hpp"
#include "protocol/udp_packet.hpp"

class FrameReassemblerManager
{
public:
    FrameReassemblerManager(size_t max_frame_size,
                            size_t payload_stride);

    bool empty() const;

    // Processing a single packet
    void pushPacket(const RxPacket& pkt,
                    size_t queue_drop_count);

    // (Implementation in the next phase)
    void pollFlush();

     void pollTimers(size_t queue_drop_now);

     // Forced flush upon termination
    void flushAll();
    void forceEmitAll(size_t queue_drop_now);

    std::function<void(const FrameResult&)> onFrameDone;

    struct
    {
        std::atomic<uint64_t> total{0};
        std::atomic<uint64_t> complete{0};
        std::atomic<uint64_t> partial{0};
    } stats_;


private:
    struct FrameEntry
    {
        FrameReassemblerV2 reassembler;
        std::chrono::steady_clock::time_point first_packet_time;
        std::chrono::steady_clock::time_point last_update;

        FrameEntry(size_t max_frame_size,
               size_t payload_stride)
        : reassembler(max_frame_size, payload_stride),
          first_packet_time(std::chrono::steady_clock::now()),
          last_update(std::chrono::steady_clock::now())
        {}

        // Queue drop count at frame start
        size_t queue_drop_at_start = 0;
    };

    size_t max_frame_size_;
    size_t payload_stride_;

    std::map<uint32_t, FrameEntry> frames_;

    void emitFrame(FrameEntry& entry,
                   FrameState final_state,
                   size_t queue_drop_now);
    
    static constexpr std::chrono::milliseconds FRAME_TIMEOUT        {3000};
    static constexpr std::chrono::milliseconds MAX_FRAME_LIFETIME   {8000};
    static constexpr std::chrono::milliseconds FRAME_IDLE_TIMEOUT   {30};
};
