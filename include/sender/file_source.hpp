#pragma once
#include <string>
#include <vector>
#include <cstdint>

class FileSource {
public:
    explicit FileSource(const std::string& path);

    bool getNextFrame(std::vector<uint8_t>& out);

private:
    std::string path_;
};
