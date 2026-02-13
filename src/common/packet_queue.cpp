
/*===============================================================*/
/*                                                               */
/*                          packet_queue.cpp                     */
/*                                                               */
/*  Thread-safe Packet Queue for UDP Receiver                    */
/*  Created on: 2026-01-10                                       */   
/*                                                               */ 
/*===============================================================*/

#include "common/packet_queue.hpp"
#include "protocol/udp_packet.hpp"

bool PacketQueue::push(std::unique_ptr<RxPacket> pkt)
{
    {
        std::lock_guard<std::mutex> lock(mtx_);

        if (q_.size() >= capacity_)
        {
            // drop newest (권장)
            dropped_++;
            return false;
        }

        q_.push(std::move(pkt));
    }

    cv_.notify_one();
    return true;
}

bool PacketQueue::try_pop(std::unique_ptr<RxPacket>& out)
{
    std::lock_guard<std::mutex> lock(mtx_);
    if (q_.empty())
        return false;

    out = std::move(q_.front());
    q_.pop();
    return true;
}


std::unique_ptr<RxPacket>
PacketQueue::pop(std::atomic<bool>& shutdown)
{
    std::unique_lock<std::mutex> lock(mtx_);

    // queue가 비어 있고, 아직 종료 신호가 아니면 대기
    while (q_.empty() && !shutdown.load())
    {
        cv_.wait(lock);
    }

    // 종료 신호 + queue 비어 있음
    if (q_.empty())
    {
        return nullptr;
    }

    auto pkt = std::move(q_.front());
    q_.pop();
    return pkt;
}



bool PacketQueue::empty() const
{
    std::lock_guard<std::mutex> lock(mtx_);
    return q_.empty();
}

size_t PacketQueue::size() const
{
    std::lock_guard<std::mutex> lock(mtx_);
    return q_.size();
}

// frame thread => timed pop with shutdown check
bool PacketQueue::pop_until(std::unique_ptr<RxPacket>& out,
                       std::chrono::milliseconds timeout,
                       const std::atomic<bool>& shutdown)
{
    std::unique_lock<std::mutex> lock(mtx_);

    // 1) 큐가 비어 있고, 종료도 아니면 기다린다
    if (q_.empty() && !shutdown.load())
    {
        cv_.wait_for(lock, timeout);
    }

    // 2) 깨어난 뒤 상태 판단
    if (!q_.empty())
    {
        // 정상적으로 패킷 수신
        out = std::move(q_.front());
        q_.pop();
        return true;
    }

    // 3) 큐는 비어 있고 shutdown이면 종료
    if (shutdown.load())
    {
        return false;
    }

    // 4) timeout + 여전히 empty
    return false;
}


std::unique_ptr<RxPacket> PacketQueue::pop_or_shutdown(std::atomic<bool>& shutdown)
{
    std::unique_lock<std::mutex> lock(mtx_);
    cv_.wait(lock, [&] {
        return !q_.empty() || shutdown.load();
    });

    if (q_.empty())
        return nullptr;

    auto pkt = std::move(q_.front());
    q_.pop();
    return pkt;
}




