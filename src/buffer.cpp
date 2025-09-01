#include "termite/buffer.hpp"

#include <sstream>

namespace termite {

Buffer::Buffer() = default;

void Buffer::set_contents(std::string text) {
    lines_.clear();
    std::stringstream ss(std::move(text));
    std::string line;
    while (std::getline(ss, line)) {
        // Keep line endings normalized; std::getline strips '\n'
        lines_.push_back(line);
    }
    if (lines_.empty()) lines_.push_back("");
}

} // namespace termite

