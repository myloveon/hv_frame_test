#include "sender/ccsds_stub_encoder.hpp"
#include "protocol/frame_header.hpp"
#include "protocol/protocol_constants.hpp"
#include <cstring>

std::vector<uint8_t>
CcsdsStubEncoder::encode(const std::vector<uint8_t>& raw,
                         uint16_t width,
                         uint16_t height,
                         uint8_t bitdepth)
{
    FrameHeader hdr{};
    hdr.magic = protocol::FRAME_MAGIC;
    hdr.version = protocol::PROTOCOL_VERSION;
    hdr.width = width;
    hdr.height = height;
    hdr.bitdepth = bitdepth;
    hdr.frame_size = raw.size();

    std::vector<uint8_t> out(sizeof(FrameHeader) + raw.size());
    std::memcpy(out.data(), &hdr, sizeof(FrameHeader));
    std::memcpy(out.data() + sizeof(FrameHeader),
                raw.data(),
                raw.size());

    return out;
}
