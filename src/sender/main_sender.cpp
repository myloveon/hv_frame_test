#include "sender/file_source.hpp"
#include "sender/ccsds_stub_encoder.hpp"
#include "sender/udp_sender.hpp"
#include "protocol/frame_header.hpp"
#include <iostream>
#include <fstream>

int main(int argc, char* argv[])
{
    if (argc < 3) {
        std::cout << "Usage: sender <ip> <port> [raw_file_path]\n";
        return -1;
    }

    const char* raw_path = (argc >= 4) ? argv[3] : "test_data/raw/gradient_1920x1080.raw";

    // quick existence check with clearer error
    std::ifstream check(raw_path, std::ios::binary);
    if (!check) {
        std::cerr << "Cannot open raw file: " << raw_path << "\n";
        return -1;
    }
    check.close();

    FileSource src(raw_path);
    CcsdsStubEncoder encoder;
    UdpSender sender(argv[1], std::stoi(argv[2]));

    std::vector<uint8_t> raw;
    if (!src.getNextFrame(raw))
    {
        std::cerr << "Failed to read frame\n";
        return -1;
    }

    auto frame =
        encoder.encode(raw, 1920, 1080, 12);

    // write sender header separately for comparison
    if (frame.size() >= sizeof(FrameHeader))
    {
        std::ofstream sh("sender_header.bin", std::ios::binary);
        sh.write(reinterpret_cast<const char*>(frame.data()), sizeof(FrameHeader));
    }

    sender.sendFrame(frame);

    return 0;
}
