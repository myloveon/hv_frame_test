#include "sender/udp_sender.hpp"
#include "protocol/udp_packet.hpp"
#include "protocol/protocol_constants.hpp"
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

UdpSender::UdpSender(const std::string& ip, uint16_t port)
    : frame_id_(0)
{
    sock_ = socket(AF_INET, SOCK_DGRAM, 0);

    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &addr_.sin_addr);
}

UdpSender::~UdpSender()
{
    close(sock_);
}

void UdpSender::sendFrame(const std::vector<uint8_t>& frame)
{
    frame_id_++;

    const size_t max_payload =
        protocol::MAX_UDP_PAYLOAD;

    uint16_t packet_count =
        (frame.size() + max_payload - 1) / max_payload;

    for (uint16_t pid = 0; pid < packet_count; pid++) {
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

        sendto(sock_,
               buffer,
               sizeof(hdr) + size,
               0,
               (struct sockaddr*)&addr_,
               sizeof(addr_));
    }
}
