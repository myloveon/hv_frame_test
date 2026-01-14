#pragma once
#include <vector>
#include <cstdint>

class CcsdsStubEncoder {
public:
    std::vector<uint8_t> encode(const std::vector<uint8_t>& raw,
                                uint16_t width,
                                uint16_t height,
                                uint8_t bitdepth);
};
