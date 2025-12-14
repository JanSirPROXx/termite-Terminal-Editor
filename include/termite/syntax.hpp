#pragma once
#include <vector>


namespace termite {

    struct SyntaxHighlight
    {
        enum class Type {
            Normal,
            Keyword,
            String,
            Comment,
            Number,
            Match,
        };

        Type type{Type::Normal};
        int start;
        int end;
    };

    std::vector<SyntaxHighlight> get_syntax_highlights(const std::string& line);


}
