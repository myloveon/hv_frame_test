/*===============================================================*/
/*                                                               */
/*                          packet_queue.hpp                     */
/*                                                               */
/*  Thread-safe Packet Queue for UDP Receiver                    */
/*                                                               */ 
/*===============================================================*/

#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <atomic>

struct RxPacket;

class PacketQueue
{
public:
    
    explicit PacketQueue(size_t capacity)
        : capacity_(capacity)
    {}

    // RX thread => push
    bool push(std::unique_ptr<RxPacket> pkt);
    

    // frame thread => non-blocking pop
    bool try_pop(std::unique_ptr<RxPacket>& out);

    // frame thread => blocking pop (Selection)
    //std::unique_ptr<RxPacket> pop();
    std::unique_ptr<RxPacket> pop(std::atomic<bool>& shutdown);

    bool empty() const;

     // observability
    size_t dropped() const { return dropped_.load(); }
    size_t size() const;

    // frame thread => timed pop with shutdown check
    bool pop_until(std::unique_ptr<RxPacket>& out,
               std::chrono::milliseconds timeout,
               const std::atomic<bool>& shutdown);


    std::unique_ptr<RxPacket> pop_or_shutdown(std::atomic<bool>& shutdown);

private:
    const size_t capacity_;

    mutable std::mutex mtx_;
    std::condition_variable cv_;
    std::queue<std::unique_ptr<RxPacket>> q_;

    std::atomic<size_t> dropped_{0};
};
