#pragma once

//
// Created by Jan Bichsel on 14.12.2025.
//

#ifndef TERMITE_DEBUG_H
#define TERMITE_DEBUG_H
#include <string>
#include <vector>

#endif //TERMITE_DEBUG_H


namespace termite
{
    extern std::vector<std::string> g_debug_lines; //global debug lines

    inline void add_debug_line(const std::string& line)
    {
        g_debug_lines.push_back(line);
    }

    inline void clear_debug_lines()
    {
        g_debug_lines.clear();
    }
}
