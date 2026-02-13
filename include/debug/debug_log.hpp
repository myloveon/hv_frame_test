/*=============================================================================*/
/*  High-Speed Debug / Trace Header File                                       */
/*=============================================================================*/


#pragma once
#include <cstdint>

/*
 * High-speed debug / trace
 * Enabled only when DEBUG_LOG_ENABLE is defined
 */

#ifdef DEBUG_LOG_ENABLE

constexpr uint16_t TRACE_FLAG_GAP = 1 << 0;


namespace debug_log {

/* extremely cheap counters */
struct Counters {
    uint64_t rx_packets;
    uint64_t rx_bytes;
    uint64_t frames;
};

struct PacketTraceEntry {
	uint64_t seq;
	uint32_t frame_id;
	uint16_t packet_id;
	uint16_t packet_count;
	uint16_t payload_size;
    uint16_t flags;         // gap detected flag
};


/* global counters */
extern Counters g;

/* inline hot-path APIs */
inline void rx_packet(uint32_t bytes)
{
    g.rx_packets++;
    g.rx_bytes += bytes;
}

inline void frame_done()
{
    g.frames++;
}


void trace_packet(const PacketTraceEntry& e);
void dump_ring(const char* path);
void mark_frame_drop(uint32_t frame_id);


} // namespace debug_log

#else   // DEBUG_LOG_ENABLE not defined

/* completely compiled out */
namespace debug_log {
inline void rx_packet(uint32_t) {}
inline void frame_done() {}
}

#endif
