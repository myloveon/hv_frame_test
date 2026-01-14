#include "sender/file_source.hpp"
#include <fstream>

FileSource::FileSource(const std::string& path)
    : path_(path)
{
}

bool FileSource::getNextFrame(std::vector<uint8_t>& out)
{
    std::ifstream ifs(path_, std::ios::binary);
    if (!ifs)
        return false;

    out.assign(std::istreambuf_iterator<char>(ifs),
               std::istreambuf_iterator<char>());

    return !out.empty();
}
