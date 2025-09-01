#include "termite/screen.hpp"

#include "termite/ansi.hpp"
#include "termite/platform.hpp"

#include <cstdio>

namespace termite {

Screen::Screen() {}
Screen::~Screen() {}

void Screen::clear() {
    std::fwrite(ansi::HIDE_CURSOR, 1, 6, stdout);
    std::fwrite(ansi::CLEAR_SCREEN, 1, 4, stdout);
    auto home = ansi::cursor_pos(1, 1);
    std::fwrite(home.c_str(), 1, home.size(), stdout);
}

void Screen::move_cursor(int row, int col) {
    auto seq = ansi::cursor_pos(row, col);
    std::fwrite(seq.c_str(), 1, seq.size(), stdout);
}

void Screen::write(const std::string& s) {
    std::fwrite(s.c_str(), 1, s.size(), stdout);
}

void Screen::flush() {
    std::fwrite(ansi::SHOW_CURSOR, 1, 6, stdout);
    std::fflush(stdout);
}

void Screen::draw_status(const std::string& status) {
    auto szOpt = platform::window_size();
    int rows = szOpt ? szOpt->rows : 24;
    int cols = szOpt ? szOpt->cols : 80;
    (void)cols;
    move_cursor(rows, 1);
    write(ansi::color256(245));
    write(status);
    write(ansi::RESET);
}

Size Screen::size() const {
    auto szOpt = platform::window_size();
    if (szOpt) return {szOpt->rows, szOpt->cols};
    return {};
}

} // namespace termite

