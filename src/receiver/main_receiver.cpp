/*=====================================================================================*/
/*                     HyperVision AGX UDP Receiver Application                        */
/*-------------------------------------------------------------------------------------*/


#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>

#include "debug/debug_log.hpp"
#include "debug/debug_stats.hpp"
#include "debug/hv_debug.hpp"

#include "protocol/udp_packet.hpp"
#include "protocol/frame_header.hpp"

#include "common/frame_writer.hpp"
#include "common/packet_queue.hpp"
#include "common/frame_reassembler_v2.hpp"
#include "common/frame_reassembler_manager.hpp"


#include <cstdlib>
#include <csignal>
#include <atomic>
#include <poll.h>


/* ================================
 * Global shutdown flag
 * ================================ */
static std::atomic<bool> g_shutdown{false};

constexpr size_t MAX_QUEUE_SIZE  = 4096 * 4;        // Queue capacity
constexpr size_t MAX_FRAME_SIZE  = 4096 * 2160 * 2; // Ex: 4K RAW
constexpr size_t PAYLOAD_STRIDE  = 1400;            // UDP payload size
static constexpr int PACKET_BURST_LIMIT = 1295;

/* ================================
 * Thread-safe Queue
 * ================================ */

 PacketQueue packet_queue(MAX_QUEUE_SIZE);



/* ================================
 *  UDP RX Thread (recvfrom ONLY)
 * ================================ */
void udp_rx_thread(int sock, PacketQueue& queue) 
{
    uint8_t buf[2048] = {0,};

    struct pollfd pfd;
    pfd.fd = sock;
    pfd.events = POLLIN;

    //static uint32_t last_frame_id   = 0;
    //static uint16_t last_packet_id  = 0;
    //static bool     have_last       = false;
    //static bool     gap_reported_in_frame = false;

    sched_param sch{};
    sch.sched_priority = 80; // root 권한
    pthread_setschedparam(pthread_self(), SCHED_FIFO, &sch);

	HV_LOGI(hv::debug::Module::RX, "##udp_rx_thread created...");

    while (!g_shutdown.load(std::memory_order_relaxed))
    {
        int ret = poll(&pfd, 1, 100); // 100ms timeout

        if (ret < 0)
        {
            // EINTR etc. => After checking the termination flag, retry
            if (errno == EINTR)
                continue;
            
            perror("poll");
            break;
        }
        if (ret == 0)
        {
            // timeout => shutdown Just checking continue
            continue;
        }

        if (pfd.revents & POLLIN)
        {
            ssize_t len = recvfrom(sock, buf, sizeof(buf), 0, nullptr, nullptr);
            if (len <= (ssize_t)sizeof(UdpPacketHeader))
                continue;
            
            auto pkt = std::make_unique<RxPacket>();
            
            // Copy header data
            std::memcpy(&pkt->hdr, buf, sizeof(UdpPacketHeader));

            // payload size sanity check
            if (pkt->hdr.payload_size > len - sizeof(UdpPacketHeader))
                continue;

            debug_log::rx_packet(len);
               
            pkt->gap_before = false;
            
    #ifdef DEBUG_LOG_ENABLE            
            debug_log::PacketTraceEntry te;
            te.seq          = 0;  // Internal overwriting (placeholder)
            te.frame_id     = pkt->hdr.frame_id;
            te.packet_id    = pkt->hdr.packet_id;
            te.packet_count = pkt->hdr.packet_count;
            te.payload_size = pkt->hdr.payload_size;
            te.flags        = 1; //te.flags        = gap_detected ? TRACE_FLAG_GAP : 0;

            debug_log::trace_packet(te);
#if 0
            if (gap_detected && !gap_reported_in_frame) 
            {
                HV_LOGW(hv::debug::Module::RX,
                        "Packet gap detected: frame=%u last_pkt=%u curr_pkt=%u",
                        pkt->hdr.frame_id,
                        last_packet_id,
                        pkt->hdr.packet_id);

                gap_reported_in_frame = true;
            }
#endif   
    #endif
            // payload Copy
            std::memcpy(pkt->payload,
                        buf + sizeof(UdpPacketHeader),
                        pkt->hdr.payload_size);

            //  Move ownership to the queue.
            queue.push(std::move(pkt));

        }
    }

    HV_LOGI(hv::debug::Module::RX, "udp_rx_thread exiting!!!");
}

/* ================================
 * Frame Worker Thread
 *  - FrameReassemblerManager
 * ================================ */
void frame_worker_thread(PacketQueue& queue)
{
    FrameReassemblerManager manager(MAX_FRAME_SIZE, PAYLOAD_STRIDE);

    manager.onFrameDone = [](const FrameResult& r)
    {
        write_frame_to_file(r.frame_data,
                            r.frame_size,
                            r.state == FrameState::PARTIAL);
    };

    auto last_timer = std::chrono::steady_clock::now();

    static auto reminder_start = std::chrono::steady_clock::time_point{};

    while (true)
    {

         //blocking pop (CPU 0% idle)
        std::unique_ptr<RxPacket> pkt;


        // ? shutdown-aware blocking pop
        bool got = queue.pop_until(
            pkt,
            std::chrono::milliseconds(1),
            g_shutdown
        );

        if (got)
        {
            size_t queue_now = queue.dropped();
            manager.pushPacket(*pkt, queue_now);
        }  
        
        //Timer processing
        auto now = std::chrono::steady_clock::now();
        if (now - last_timer >= std::chrono::milliseconds(5))
        {
            manager.pollTimers(queue.dropped());
            last_timer = now;
        }

        //Termination condition
        if (g_shutdown.load() 
                && queue.empty())
        {
            if (reminder_start.time_since_epoch().count() == 0)
                reminder_start = std::chrono::steady_clock::now();

            if (std::chrono::steady_clock::now() - reminder_start
                > std::chrono::milliseconds(100))
                break;
        }

        // CPU yield
        std::this_thread::yield();
    }
    
    // Forced flush at termination 
    HV_LOGI(hv::debug::Module::FRAME, "frame_worker_thread exiting");

    manager.flushAll();

}

/* ================================
 * Signal Handler
 * ================================ */


static void on_signal(int)
{
#ifdef DEBUG_LOG_ENABLE
    //debug_log::dump_ring("packet_trace.log");
#endif
    g_shutdown.store(true, std::memory_order_relaxed);
}


static void on_exit_dump()
{
#ifdef DEBUG_LOG_ENABLE
    debug_log::dump_ring("packet_trace.log");
#endif
}


/* ================================
 * main
 * ================================ */
int main(int argc, char* argv[])
 {
    if (argc < 2) {
        std::cerr << "Usage: receiver <port>\n";
        return -1;
    }

    int port = std::stoi(argv[1]);

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket");
        return -1;
    }

    int rcvbuf = 16 * 1024 * 1024; // 16MB 이상
    if (setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &rcvbuf, sizeof(rcvbuf)) < 0)
    {
        perror("setsockopt(SO_RCVBUF)");
    }
   
    int reuse = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
                &reuse, sizeof(reuse)) < 0)
    {
        perror("setsockopt(SO_REUSEADDR)");
    }


    std::signal(SIGTERM, on_signal);
    std::signal(SIGINT,  on_signal);

    std::atexit(on_exit_dump);


    /* Debug log Initialisation */
    hv::debug::init();
    hv::debug::setLevel(hv::debug::Level::Debug);
    hv::debug::enable(hv::debug::Module::RX
                     | hv::debug::Module::FRAME);

    HV_LOGI(hv::debug::Module::RX, "##hv_debug init OK");

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, (sockaddr*)&addr, sizeof(addr)) < 0)
    {
        perror("bind");
        close(sock);
        return -1;
    }

    std::thread rx_thread(udp_rx_thread, sock, std::ref(packet_queue));
    std::thread frame_thread(frame_worker_thread, std::ref(packet_queue));
    
    
    // 4. The main thread waits until a termination request is received.
    while (!g_shutdown.load(std::memory_order_relaxed))
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    HV_LOGI(hv::debug::Module::RX, "shutdown requested");

    // 5. RX End
    if (rx_thread.joinable())
        rx_thread.join();

    // 6. PROC End (Queue emptying + partial flush included)
    if (frame_thread.joinable())
        frame_thread.join();

    // 7. After all threads have terminated and trace dump
#ifdef DEBUG_LOG_ENABLE
    debug_log::dump_ring("packet_trace.log");
#endif

    close(sock);
    return 0;
}

