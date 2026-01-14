#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <netinet/in.h>
#include <sys/socket.h>

class UdpSender {
public:
    UdpSender(const std::string& ip, uint16_t port);
    ~UdpSender();

    void sendFrame(const std::vector<uint8_t>& frame);

private:
    int sock_;
    uint32_t frame_id_;
    struct sockaddr_in addr_;
};
