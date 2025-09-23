#include "termite/buffer.hpp"

#include <sstream>

namespace termite {

Buffer::Buffer() {
    // Ensure there is always at least one line
    lines_.push_back("");
}

void Buffer::set_contents(std::string text) {
    lines_.clear();
    std::stringstream ss(std::move(text));
    std::string line;
    while (std::getline(ss, line)) {
        // Keep line endings normalized; 
        //std::getline strips '\n'
        lines_.push_back(line);
    }
    if (lines_.empty()) lines_.push_back(""); // Ensure there is always at least one line
}

void Buffer::insert_char(size_t row, size_t col, char ch) {
    if (row >= lines_.size()) return;
    auto& s = lines_[row];
    if (col > s.size()) col = s.size();
    s.insert(s.begin() + static_cast<std::ptrdiff_t>(col), ch);
}

void Buffer::delete_char(size_t row, size_t col) {
    if (row >= lines_.size()) return;
    auto& s = lines_[row];
    if (col >= s.size()) return;
    s.erase(s.begin() + static_cast<std::ptrdiff_t>(col));
}

void Buffer::split_line(size_t row, size_t col) {
    if (row >= lines_.size()) return;
    auto& s = lines_[row];
    if (col > s.size()) col = s.size();
    std::string right = s.substr(col);
    s.erase(col);
    lines_.insert(lines_.begin() + static_cast<std::ptrdiff_t>(row + 1), std::move(right));
}

void Buffer::join_with_next(size_t row) {
    if (row + 1 >= lines_.size()) return;
    lines_[row] += lines_[row + 1];
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(row + 1));
}

void Buffer::delete_line(size_t row) {
    if (row >= lines_.size()) return;
    if (lines_.size() == 1) {
        lines_[0].clear();
        return;
    }
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(row));
}

}
