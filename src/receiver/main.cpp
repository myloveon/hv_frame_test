#include <iostream>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

#include "protocol/udp_packet.hpp"
#include "common/frame_reassembler.hpp"
#include "protocol/frame_header.hpp"

int main(int argc, char* argv[])
{
    if (argc < 2) {
        std::cout << "Usage: receiver <port>\n";
        return -1;
    }

    int port = std::stoi(argv[1]);

    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(sock, (sockaddr*)&addr, sizeof(addr));

    FrameReassembler reassembler;

    uint8_t buffer[2048];

    while (true) {
        ssize_t len = recvfrom(sock, buffer, sizeof(buffer), 0, nullptr, nullptr);
        if (len <= 0)
            continue;

        if (len < sizeof(UdpPacketHeader))
            continue;

        UdpPacketHeader hdr{};
        std::memcpy(&hdr, buffer, sizeof(hdr));

        uint8_t* payload = buffer + sizeof(hdr);

        std::cout << "Recv pkt: frame_id=" << hdr.frame_id
              << " packet_id=" << hdr.packet_id
              << " packet_count=" << hdr.packet_count
              << " payload_size=" << hdr.payload_size
              << " len=" << len << "\n";

        reassembler.pushPacket(hdr, payload);

        if (reassembler.frameComplete()) {
            auto frame = reassembler.popFrame();
            std::cout << "Frame received, size = "
                      << frame.size() << " bytes\n";

             //Temporary dump (STEP 4 core)
            std::cout << "Frame received, writing file...\n";
            // write full received frame
            FILE* fp = fopen("received_frame.bin", "wb");
            if (!fp) {
                std::perror("fopen");
            } else {
                size_t written = fwrite(frame.data(), 1, frame.size(), fp);
                std::cout << "Wrote " << written << " bytes to received_frame.bin\n";
                fclose(fp);
            }

            // also extract header and raw payload separately for comparison
            if (frame.size() >= sizeof(FrameHeader)) {
                // header
                FILE* hh = fopen("received_header.bin", "wb");
                if (hh) {
                    fwrite(frame.data(), 1, sizeof(FrameHeader), hh);
                    fclose(hh);
                }

                // raw payload without header
                FILE* rf = fopen("received_raw.bin", "wb");
                if (rf) {
                    fwrite(frame.data() + sizeof(FrameHeader), 1,
                           frame.size() - sizeof(FrameHeader), rf);
                    fclose(rf);
                }
            }
        }
    }

    close(sock);
    return 0;
}
