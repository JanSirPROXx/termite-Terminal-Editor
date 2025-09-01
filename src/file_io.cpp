#include "termite/file_io.hpp"

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

}

