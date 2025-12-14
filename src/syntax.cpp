#include "termite/syntax.hpp"
#include <iostream>

namespace termite
{
    struct SyntaxHighlight;
    std::vector<SyntaxHighlight> get_syntax_highlights(const std::string& line)
    {
        // Dummy implementation: highlight keywords "int", "return"
        std::vector<SyntaxHighlight> highlights;
        size_t pos = 0;
        while (pos < line.size())
        {
            if (line.compare(pos, 3, "int") == 0)
            {
                highlights.push_back({SyntaxHighlight::Type::Keyword, (int)pos, (int)(pos + 3)});
                pos += 3;
            }
            else if (line.compare(pos, 6, "return") == 0)
            {
                highlights.push_back({SyntaxHighlight::Type::Keyword, (int)pos, (int)(pos + 6)});
                pos += 6;
            }
            else
            {
                //This really ugly and totaly not efficient, todo: improve syntax highlighting algorithm
                highlights.push_back({SyntaxHighlight::Type::Normal, (int)pos, (int)(pos + 1)});
                pos++;
            }
        }
        // highlights.push_back({SyntaxHighlight::Type::Keyword, (int)pos, (int)pos});

        return highlights;
    }

}