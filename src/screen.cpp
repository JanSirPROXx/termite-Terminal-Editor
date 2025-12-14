#include "termite/screen.hpp"

#include "termite/ansi.hpp"
#include "termite/platform.hpp"

#include <cstdio>

namespace termite
{
    Screen::Screen()
    {
    }

    Screen::~Screen()
    {
    }

    void Screen::clear()
    {
        std::fwrite(ansi::HIDE_CURSOR, 1, 6, stdout);
        std::fwrite(ansi::CLEAR_SCREEN, 1, 4, stdout);
        auto home = ansi::cursor_pos(1, 1);
        std::fwrite(home.c_str(), 1, home.size(), stdout);
    }

    void Screen::move_cursor(int row, int col)
    {
        auto seq = ansi::cursor_pos(row, col);
        //low level terminal output
        std::fwrite(seq.c_str(), 1, seq.size(), stdout);
    }

    void Screen::write(const std::string& s)
    {
        std::fwrite(s.c_str(), 1, s.size(), stdout);
    }

    void Screen::flush()
    {
        std::fwrite(ansi::SHOW_CURSOR, 1, 6, stdout);
        std::fflush(stdout);
    }

    void Screen::draw_status(const std::string& status)
    {
        auto szOpt = platform::window_size();
        int rows = szOpt ? szOpt->rows : 24;
        int cols = szOpt ? szOpt->cols : 80;
        (void)cols;
        move_cursor(rows, 1);
        write(ansi::color256(245));
        write(status);
        write(ansi::RESET);
    }

    void Screen::draw_line_numbers(std::size_t line_size)
    {
        for (std::size_t i = 0; i < line_size; ++i)
        {
            move_cursor(i, 0);
            write(ansi::color256(245));
            write(std::to_string(i));
        }
        write(ansi::RESET);
    }

    std::size_t get_size_of_biggest_debug_line(const std::vector<std::string>& lines)
    {
        std::size_t maxSize = 0;
        for (const std::string& line : lines)
        {
            if (maxSize < line.size())
            {
                maxSize = line.size();
            }
        }

        return maxSize;
    }

    // debug window
    //since g_debug_lines is global we shouldn't have to pass it but i do it anyway, in case we remove the debugging. So we could use this method to also render other information :)
    void Screen::draw_debug_window(const std::vector<std::string>& lines)
    {
        if (lines.empty()) return;
        int startRow = 2;

        std::size_t  col_index = platform::window_size()->cols - get_size_of_biggest_debug_line(lines);
        for (size_t i = 0; i < lines.size(); ++i)
        {
            move_cursor(startRow + i, col_index); //coll must be max width - size of biggest debug line;
            write(ansi::bg_color256(52)); //dark red background
            write(ansi::color256(231)); //white text
            write(lines[i]);
            write(ansi::RESET);
        }
    }

    Size Screen::size() const
    {
        auto szOpt = platform::window_size();
        if (szOpt)
            return {szOpt->rows, szOpt->cols};
        return {};
    }
} // namespace termite
