#include "termite/file_io.hpp"
#include "termite/buffer.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace termite::file_io {

std::string read_file(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("failed to open file");
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

bool save_file(const Buffer& buffer, const std::string& path) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) return false;
    const auto& lines = buffer.lines();
    for (size_t i = 0; i < lines.size(); ++i) {
        out.write(lines[i].data(), static_cast<std::streamsize>(lines[i].size()));
        if (i + 1 < lines.size()) out.put('\n');
    }
    return static_cast<bool>(out);
}

}

