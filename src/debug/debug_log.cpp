/*================================================================================*/
/*  Debug Statistics Source File                                                  */
/*================================================================================*/


#ifdef DEBUG_LOG_ENABLE
#include "debug/debug_log.hpp"
#include <atomic>
#include <cstdio>
//#include <algorithm>

namespace debug_log {

constexpr uint32_t TRACE_RING_SIZE = 4096;

static uint32_t g_last_dropped_frame = 0;

Counters g{};

static PacketTraceEntry g_ring[TRACE_RING_SIZE];
static std::atomic<uint64_t> g_write_seq{0};

void trace_packet(const PacketTraceEntry& e)
{
	uint64_t seq = g_write_seq.fetch_add(1, std::memory_order_relaxed);
	g_ring[seq & (TRACE_RING_SIZE - 1)] = e;
}

void dump_ring(const char* path)
{
    FILE* fp = std::fopen(path, "w");
    if (!fp) return;

    // The total recorded to date seq
    uint64_t end = g_write_seq.load(std::memory_order_relaxed);
    uint64_t start = (end > TRACE_RING_SIZE) ? (end - TRACE_RING_SIZE) : 0;

    std::fprintf(fp, "# seq frame_id packet_id/packet_count payload\n");

    for (uint64_t s = start; s < end; ++s) 
	{
        const auto& e = g_ring[s & (TRACE_RING_SIZE - 1)];
        std::fprintf(fp,
            //"%lu %u %u/%u %u %s\n",
            "%lu %u %u/%u %u [%d]\n",
            s,
            e.frame_id,
            e.packet_id,
            e.packet_count,
            e.payload_size,
			e.flags);//(e.flags & TRACE_FLAG_GAP) ? "GAP" : "-");
    }

    std::fclose(fp);
}


void mark_frame_drop(uint32_t frame_id)
{
    g_last_dropped_frame = frame_id;
}



}		//namespace debug_log
#endif
