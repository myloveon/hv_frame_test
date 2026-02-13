/*---------------------------------------------------------------------------*/
/*                                                                           */
/*                    frame_reassembler_manager.cpp                          */
/*                                                                           */
/*  Frame Reassembler Manager for UDP Receiver                               */
/*  Created on: 2025-01-10                                                   */
/*                                                                           */
/*---------------------------------------------------------------------------*/

#include "common/frame_writer.hpp"
#include "common/frame_reassembler_manager.hpp"
#include "common/frame_result.hpp"

#include "debug/debug_log.hpp"
#include "debug/debug_stats.hpp"
#include "debug/hv_debug.hpp"


FrameReassemblerManager::FrameReassemblerManager(size_t max_frame_size,
                                                 size_t payload_stride)
    : max_frame_size_(max_frame_size),
      payload_stride_(payload_stride)
{
}

bool FrameReassemblerManager::empty() const
{
    return frames_.empty();
}


/*-------------------------------------------*/
/* Processing a single packet                */
/*-------------------------------------------*/
void FrameReassemblerManager::pushPacket(const RxPacket& pkt, size_t queue_drop_count)
{
    uint32_t frame_id = pkt.hdr.frame_id;
    auto now = std::chrono::steady_clock::now();

    auto it = frames_.find(frame_id);

    if (it == frames_.end())
    {
        FrameEntry entry(max_frame_size_, payload_stride_);
        entry.reassembler.startNewFrame(frame_id, pkt.hdr.packet_count);
        entry.first_packet_time = now;
        entry.last_update = now;

        entry.queue_drop_at_start = queue_drop_count;

        it = frames_.emplace(frame_id, std::move(entry)).first;
    }

    FrameEntry& entry = it->second;
    FrameReassemblerV2& fr = entry.reassembler;

    fr.pushPacket(pkt.hdr, pkt.payload, pkt.gap_before);
    entry.last_update = now;

    #ifdef HV_DEBUG_ENABLED
    HV_LOGI(hv::debug::Module::FRAME, "[RX ] frame=%u pkt=%u/%u gap=%u",
                        frame_id,
                        pkt.hdr.packet_id,
                        pkt.hdr.packet_count,
                        pkt.gap_before);    
    #endif

#ifdef DEBUG_LOG_ENABLE
	{
	    debug_log::PacketTraceEntry te{};
	    te.frame_id = fr.frameId();
	    te.packet_id = fr.receivedPackets();
	    te.packet_count = fr.expectedPackets();
		te.payload_size = pkt.hdr.payload_size;
	    te.flags = 3;

	    debug_log::trace_packet(te);
	}
#endif
}



void FrameReassemblerManager::pollFlush()
{
    // Check for completed or timed-out frames
    auto now = std::chrono::steady_clock::now();

    for (auto it = frames_.begin(); it != frames_.end(); )
    {
        FrameEntry& entry = it->second;
        FrameReassemblerV2& fr = entry.reassembler;

        uint16_t expected = fr.expectedPackets();
        uint16_t received = fr.receivedPackets();
        bool gap          = fr.hasGap();
        
        bool is_complete = fr.isFrameComplete();
        bool lifetime_expired = (now - entry.first_packet_time) > MAX_FRAME_LIFETIME;

        //Successful Frame
        if (is_complete && !gap)
        {
             HV_LOGI(hv::debug::Module::FRAME, "[OK ] frame=%u rx=%u/%u",
                                fr.frameId(),
                                received,
                                expected);

            FrameResult r = fr.makeResult(FrameState::COMPLETE);

            write_frame_to_file(fr.getFrameData(),
                                fr.getFrameSize(),
                                false);

            it = frames_.erase(it);
            continue;                            
        }

        //File Frame due to timeout or lifetime expiry
        if(lifetime_expired)
        {
         
            HV_LOGW(hv::debug::Module::FRAME,
                    "[END] frame=%u rx=%u/%u gap=%d",
                    fr.frameId(),
                    received,
                    expected,
                    gap);

            FrameResult r = fr.makeResult(FrameState::PARTIAL);

            write_frame_to_file(fr.getFrameData(),
                                fr.getFrameSize(),
                                true);

            it = frames_.erase(it);
            continue;
        }

        //etc: still waiting
        HV_LOGD(hv::debug::Module::FRAME,
                "[WAIT] frame=%u rx=%u/%u gap=%d age=%lldms",
                fr.frameId(),
                received,
                expected,
                gap,
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    now - entry.first_packet_time).count());

        ++it;
    }
}

/*-------------------------------------------*/
/* Forced flush upon termination             */
/*-------------------------------------------*/
void FrameReassemblerManager::flushAll()
{
    for (auto& kv : frames_)
    {
        auto& fr = kv.second.reassembler;

        if (fr.hasAnyPacket())
        {
            write_frame_to_file(fr.getFrameData(),
                                fr.getFrameSize(),
                                true);

            HV_LOGW(hv::debug::Module::FRAME,
                    "[FLUSH - ALL] frame=%u rx=%u/%u",
                    fr.frameId(),
                    fr.receivedPackets(),
                    fr.expectedPackets());
        }
    }
    frames_.clear();
}


/*-------------------------------------------*/
/* Emit a completed frame                    */
/*-------------------------------------------*/
void FrameReassemblerManager::emitFrame(FrameEntry& entry,
                                        FrameState final_state,
                                        size_t queue_drop_now)
{
    FrameReassemblerV2& fr = entry.reassembler;

    FrameResult r{};
    r.frame_id = fr.frameId();
    r.state    = final_state;

    r.expected_packets = fr.expectedPackets();
    r.received_packets = fr.receivedPackets();

    r.complete  = (r.received_packets == r.expected_packets);
    r.corrupted = fr.hasGap() || fr.hasCorruption();

    //queue pressure 판정 (프레임 시작 이후 drop 발생했는가)
    r.queue_pressure = (queue_drop_now > entry.queue_drop_at_start);

    r.frame_data = fr.getFrameData();
    r.frame_size = fr.getFrameSize();

	// 상태 정리 (내부용)
	//r.state = FrameState::EMITTED;

    stats_.total++;
   
    if (final_state == FrameState::PARTIAL)
    {
        HV_LOGW(hv::debug::Module::FRAME, "[GAP PARTIAL] frame=%u missing=%u/%u",
                                r.frame_id,
                                r.expected_packets - r.received_packets,
                                r.expected_packets);

        stats_.partial++;

        #ifdef DEBUG_LOG_ENABLE
        for (uint16_t i = 0; i < fr.expectedPackets(); ++i)
        {
            if (!fr.hasPacket(i))
            {
                debug_log::PacketTraceEntry te{};
                te.frame_id = fr.frameId();
                te.packet_id = i;
                te.packet_count = fr.expectedPackets();
                te.flags = 2;

                debug_log::trace_packet(te);
            }
        }
        #endif
    }
    else
    {
        HV_LOGI(hv::debug::Module::FRAME, "[EMIT COMPLETE] frame=%u packets=%u",
                                r.frame_id,
                                r.received_packets);
        stats_.complete++;
    }

     
    //if ((stats_.total % 100) == 0)
    {
        HV_LOGI(hv::debug::Module::FRAME, "[STATS] total=%llu complete=%llu partial=%llu success=%.2f%%",
            stats_.total.load(),
            stats_.complete.load(),
            stats_.partial.load(),
            stats_.total ? 100.0 * stats_.complete / stats_.total : 0.0);
    }


    if (onFrameDone)
        onFrameDone(r);
}


/*-------------------------------------------*/
/* Poll timers for frame timeouts            */
/*-------------------------------------------*/
void FrameReassemblerManager::pollTimers(size_t queue_drop_now)
{
    auto now = std::chrono::steady_clock::now();

    for (auto it = frames_.begin(); it != frames_.end(); )
    {
        FrameEntry& entry = it->second;
        FrameReassemblerV2& fr = entry.reassembler;

        auto since_last = now - entry.last_update;
        auto age = now - entry.first_packet_time;

              
        if (fr.receivedPackets() == fr.expectedPackets())
        {
            emitFrame(entry, FrameState::COMPLETE, queue_drop_now);
            it = frames_.erase(it);
            continue;
        }

        bool hard_timeout = (age > MAX_FRAME_LIFETIME);
        bool idle_timeout = (since_last > FRAME_IDLE_TIMEOUT);
       
        // Completed successfully
        if  (!idle_timeout 
            && !hard_timeout)
        {
            ++it;
            continue;
        }

        FrameState state = (fr.receivedPackets() != fr.expectedPackets())
                 ? FrameState::COMPLETE
                 : FrameState::PARTIAL;

        emitFrame(entry, state, queue_drop_now);
        it = frames_.erase(it);
    }
}


void FrameReassemblerManager::forceEmitAll(size_t queue_drop_now)
{
    for (auto it = frames_.begin(); it != frames_.end(); )
    {
        FrameEntry& entry = it->second;
        FrameReassemblerV2& fr = entry.reassembler;

        FrameState state =
            (fr.receivedPackets() == fr.expectedPackets())
                ? FrameState::COMPLETE
                : FrameState::PARTIAL;

        emitFrame(entry, state, queue_drop_now);
        it = frames_.erase(it);
    }
}



