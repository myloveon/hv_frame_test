#include "sender/udp_sender.hpp"
#include "protocol/udp_packet.hpp"
#include "protocol/protocol_constants.hpp"
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <fcntl.h>
#include <poll.h>
#include <errno.h>


UdpSender::UdpSender(const std::string& ip, uint16_t port)
    : frame_id_(0)
{
    sock_ = socket(AF_INET, SOCK_DGRAM, 0);

     //Holoscan-style: non-blocking I/O
    int flags = fcntl(sock_, F_GETFL, 0);
    fcntl(sock_, F_SETFL, flags | O_NONBLOCK);

    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &addr_.sin_addr);
}

UdpSender::~UdpSender()
{
    close(sock_);
}

bool UdpSender::waitWritable()
{
    struct pollfd pfd;
    pfd.fd = sock_;
    pfd.events = POLLOUT;

    while (true)
    {
        int ret = poll(&pfd, 1, -1);  // None sleep
        if (ret < 0) 
        {
            if (errno == EINTR)
                continue;   // signal => Poll retry

            perror("poll");
            return false;
        }

        if (pfd.revents & POLLOUT)
            return true;
    }
}


void UdpSender::sendFrame(const std::vector<uint8_t>& frame)
{
    frame_id_++;

    const size_t max_payload =
        protocol::MAX_UDP_PAYLOAD;

    uint16_t packet_count =
        (frame.size() + max_payload - 1) / max_payload;

    std::cout << "[TX] start frame frame_id=" << frame_id_
              << " packets=" << packet_count << "\n";

    for (uint16_t pid = 0; pid < packet_count; pid++) 
    {
        size_t offset = pid * max_payload;
        size_t size =
            std::min(max_payload, frame.size() - offset);

        UdpPacketHeader hdr{};
        hdr.frame_id = frame_id_;
        hdr.packet_id = pid;
        hdr.packet_count = packet_count;
        hdr.payload_size = size;

        
        uint8_t buffer[sizeof(hdr) + max_payload];
        std::memcpy(buffer, &hdr, sizeof(hdr));
        std::memcpy(buffer + sizeof(hdr),
                    frame.data() + offset,
                    size);

        //Key: Wait until the socket is writable
        if (!waitWritable())
        {
            std::cout << "\n[TX] WaitWritable frame id:" << hdr.frame_id << " Error !!!!\n";
            break;
        }
        
        if (pid == 0) 
        {
            std::cout << "\n[TX] start frame \n";
        }
        else if (pid == packet_count - 1)
        {
            std::cout << "\n[TX] end frame \n";
        }
                   
        ssize_t ret = sendto(sock_,
                            buffer,
                            sizeof(hdr) + size,
                            0,
                            (struct sockaddr*)&addr_,
                            sizeof(addr_));

        if (ret < 0)
        {
            if (errno == EAGAIN
                || errno == EWOULDBLOCK)
            {
                pid--;          // retry sending this packet
                std::cout << "[TX] Retry Send frame_id =" << frame_id_ << " !!!\n";
                continue;
            }

            perror("sendto");
            break;
        }

         std::cout << "[TX] send packet: "
                << "frame_id=" << hdr.frame_id
                << " packet_id=" << hdr.packet_id
                << " frame_size=" << frame.size()
                << "/" << hdr.packet_count
                << " payload=" << hdr.payload_size
                << "\n";
    }

    std::cout << "[TX] end frame frame_id=" << frame_id_ << "\n";
}
