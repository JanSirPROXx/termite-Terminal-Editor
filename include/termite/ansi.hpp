#pragma once

#include <string>

namespace termite::ansi { //ansi engine

inline constexpr const char* CLEAR_SCREEN = "\x1b[2J";
inline constexpr const char* CLEAR_LINE = "\x1b[2K";
inline constexpr const char* RESET = "\x1b[0m";
inline constexpr const char* HIDE_CURSOR = "\x1b[?25l";
inline constexpr const char* SHOW_CURSOR = "\x1b[?25h";
inline constexpr const char* REVERSE = "\x1b[7m";
inline constexpr const char* BOLD = "\x1b[1m";

inline std::string cursor_pos(int row, int col) {
    return "\x1b[" + std::to_string(row) + ";" + std::to_string(col) + "H";
}

inline std::string color256(int idx) {
    return "\x1b[38;5;" + std::to_string(idx) + "m";
}

inline std::string bg_color256(int idx) {
    return "\x1b[48;5;" + std::to_string(idx) + "m";
}

} // namespace termite::ansi
