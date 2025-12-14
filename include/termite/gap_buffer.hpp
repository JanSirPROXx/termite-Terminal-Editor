#pragma once

#include <string>
#include <vector>

namespace termite {

// Placeholder for a future gap-buffer implementation.
class GapBuffer {
public:
    GapBuffer() = default;
    void insert(char) {}
    void erase() {}
    std::string to_string() const { return {}; }
};

} // namespace termite

