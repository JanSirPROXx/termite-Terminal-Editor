#pragma once

#include <string>
#include <vector>
#include "termite/syntax.hpp"

namespace termite {

struct Size { int rows{24}; int cols{80}; };

class Screen {
public:
    Screen();
    ~Screen();

    void clear();
    void move_cursor(int row, int col);
    void write(const std::string& s);
    void flush();
    void draw_status(const std::string& status);
    void draw_line_numbers(std::size_t line_size);
    void draw_debug_window(const std::vector<std::string>& lines);
    void write_with_syntax_highlighting(const std::vector<SyntaxHighlight>& highlights,
                                        const std::string& text);
    Size size() const;
};

} // namespace termite

