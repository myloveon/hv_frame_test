
/*===============================================================*/
/*                                                               */
/*                          frame_result.cpp                     */
/*                                                               */
/*  Frame Result Implementation                                  */
/*  Created on: 2026-01-10                                       */   
/*                                                               */ 
/*===============================================================*/

#include "common/frame_result.hpp"
#include "debug/hv_debug.hpp"

void FrameStreamStats::update(const FrameResult& r)
{
    frames_total++;

    packets_expected += r.expected_packets;
    packets_received += r.received_packets;

    if (r.state == FrameState::COMPLETE)
    {
        frames_complete++;
    }
    else if (r.state == FrameState::PARTIAL)
    {
        frames_partial++;

        if (r.queue_pressure)
            partial_due_to_queue++;
        else
            partial_due_to_gap++;
    }
}

void FrameStreamStats::log() const
{
    uint64_t total = frames_total.load();
    if (total == 0)
        return;

    HV_LOGI(hv::debug::Module::FRAME,
        "[STATS] frames=%llu ok=%llu partial=%llu "
        "queue=%llu gap=%llu pkt=%llu/%llu",
        total,
        frames_complete.load(),
        frames_partial.load(),
        partial_due_to_queue.load(),
        partial_due_to_gap.load(),
        packets_received.load(),
        packets_expected.load());
}


