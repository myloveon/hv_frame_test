/*================================================================================*/
/*  Debug Statistics Header File                                                      */
/*================================================================================*/


#pragma once
#include <atomic>
#include <cstdint>

namespace hv::stats {

/* A frequently accessed counter */
struct Counters {
    std::atomic<uint64_t> rx_packets{0};
    std::atomic<uint64_t> rx_bytes{0};

    std::atomic<uint64_t> frames_started{0};
    std::atomic<uint64_t> frames_completed{0};
    std::atomic<uint64_t> frames_dropped{0};
};

/* Global single instance */
extern Counters g;

} // namespace hv::stats
