#pragma once

#include <string>
#include <vector>

namespace termite {

class Buffer {
public:
    Buffer();

    void set_contents(std::string text);
    const std::vector<std::string>& lines() const { return lines_; }
    const std::vector<std::string>& syntaxLines() const { return syntaxLines_; }

    size_t line_count() const { return lines_.size(); }
    size_t line_length(size_t row) const { return row < lines_.size() ? lines_[row].size() : 0; }
    void insert_char(size_t row, size_t col, char ch);
    // Delete character at postion, -> not if pos.y = line_length
    void delete_char(size_t row, size_t col);
    // Split line at position: current line becomes [0,col), new line inserted with [col, end)
    void split_line(size_t row, size_t col);
    // Append next line to current
    void join_with_next(size_t row);
    // Delete entire line, -> no empty line left. Like in vs code :)
    void delete_line(size_t row);


private:
    std::vector<std::string> lines_;
    std::vector<std::string> syntaxLines_;
};

} // namespace termite
