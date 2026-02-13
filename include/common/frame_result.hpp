/*-------------------------------------------*/
/*                                           */
/*  frame_state.hpp                         */
/*                                           */
/*  Frame State Definition                   */
/*                                           */
/* ------------------------------------------*/


#pragma once

#include <cstdint>
#include <cstddef>
#include <atomic>
//#include "frame_state.hpp"

//----------------------------------------------
// Frame State Enum
//----------------------------------------------
enum class FrameState
{
    RECEIVING,   // 패킷 수신 중
    COMPLETE,    // 모든 패킷 수신 완료 (gap 없음)
    PARTIAL,     // gap 존재 or 수명 초과
    EMITTED      // 결과 외부로 전달됨 (종료 상태)
};


//----------------------------------------------
// Frame Result Structure
//----------------------------------------------
struct FrameResult
{
    uint32_t frame_id;

    FrameState state;

    bool complete;      // expected_packets == received_packets
    bool corrupted;     // gap or CRC error 포함 여부

    bool queue_pressure;   // queue overflow 영향 여부

    uint16_t expected_packets;
    uint16_t received_packets;

    size_t   frame_size;
    const uint8_t* frame_data;
};


//----------------------------------------------
// Frame Stream Statistics
//----------------------------------------------
struct FrameStreamStats
{
    std::atomic<uint64_t> frames_total{0};
    std::atomic<uint64_t> frames_complete{0};
    std::atomic<uint64_t> frames_partial{0};

    std::atomic<uint64_t> partial_due_to_queue{0};
    std::atomic<uint64_t> partial_due_to_gap{0};

    std::atomic<uint64_t> packets_expected{0};
    std::atomic<uint64_t> packets_received{0};

    
};

//----------------------------------------------