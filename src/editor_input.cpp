#include "termite/editor.hpp"

#include "termite/buffer.hpp"
#include "termite/input.hpp"
#include "termite/file_io.hpp"
#include "termite/screen.hpp"

#include <algorithm>
#include <cctype>
#include <string>

namespace termite
{

    std::string Editor::prompt_input(const std::string &prompt, const std::string &initial)
    {
        std::string input = initial;
        while (true)
        {
            screen_->draw_status(prompt + input);
            screen_->flush();
            int k = input::read_key();
            if (k == input::KEY_ENTER)
            {
                return input;
            }
            else if (k == input::KEY_CTRL_C || k == 27 /*ESC*/)
            {
                return std::string();
            }
            else if (k == input::KEY_BACKSPACE)
            {
                if (!input.empty())
                    input.pop_back();
            }
            else if (k >= 32 && k <= 126)
            {
                input.push_back(static_cast<char>(k));
            }
        }
    }

    void Editor::delete_selection()
    {
        if (!selection_active())
            return;
        // Normalize selection
        int aL = anchor_cy_ - 1, aC = anchor_cx_ - 1;
        int cL = cy_ - 1, cC = cx_ - 1;
        if (aL > cL || (aL == cL && aC > cC))
        {
            std::swap(aL, cL);
            std::swap(aC, cC);
        }
        int line_count = (int)buffer_->line_count();
        if (line_count <= 0)
        {
            selecting_ = false;
            return;
        }
        aL = std::clamp(aL, 0, line_count - 1);
        cL = std::clamp(cL, 0, line_count - 1);
        aC = std::clamp(aC, 0, (int)buffer_->line_length((size_t)aL));
        cC = std::clamp(cC, 0, (int)buffer_->line_length((size_t)cL));

        if (aL == cL)
        {
            // Delete [aC, cC) on the same line
            int count = cC - aC;
            for (int i = 0; i < count; ++i)
            {
                buffer_->delete_char((size_t)aL, (size_t)aC);
            }
        }
        else
        {
            // Trim end line prefix [0, cC)
            for (int i = 0; i < cC; ++i)
            {
                buffer_->delete_char((size_t)cL, 0);
            }
            // Trim start line suffix from aC to end
            while ((int)buffer_->line_length((size_t)aL) > aC)
            {
                buffer_->delete_char((size_t)aL, (size_t)aC);
            }
            // Join lines from aL with next until cL collapses
            while (aL + 1 <= cL)
            {
                buffer_->join_with_next((size_t)aL);
                --cL;
            }
        }
        cy_ = aL + 1;
        cx_ = aC + 1;
        selecting_ = false;
        modified_ = true;
    }

    bool Editor::handle_input(int key)
    {
        if (key == input::KEY_CTRL_F)
        {
            start_search();
            return true;
        }
        if (key == input::KEY_CTRL_V)
        {
            paste_from_clipboard();
            return true;
        }
        // if (key == input::KEY_CTRL_G)
        // {
        //     debug_note("Ctrl-G");
        //     return true;
        // }
        // While in search mode, Up/Down navigate results
        if (searching_ && (key == input::KEY_UP || key == input::KEY_DOWN))
        {
            if (!search_matches_.empty())
            {
                int n = (int)search_matches_.size();
                if (search_index_ < 0)
                    search_index_ = 0;
                if (key == input::KEY_UP)
                    search_index_ = (search_index_ - 1 + n) % n;
                else
                    search_index_ = (search_index_ + 1) % n;
                jump_to_match(search_index_);
            }
            return true;
        }
        if (key == input::KEY_CTRL_Q)
        {
            return false;
        }
        if (key == input::KEY_CTRL_C) // Actually CTRL + SHIFT + C -> buggy (probably becaus i changed my terminal settings to copy with ctrl+c instead of ctrl+shift+c so it switched them)
        {
            copy_selection_to_clipboard();
            return true;
        }
        if (key == input::KEY_CTRL_S)
        {
            std::string target = filename_.empty() ? std::string("") : filename_;
            if (target.empty())
            {
                std::string entered = prompt_input("Save As: ", filename_);
                if (entered.empty())
                {
                    status_ = "Save canceled";
                    return true;
                }
                target = entered;
            }
            if (file_io::save_file(*buffer_, target))
            {
                filename_ = target;
                modified_ = false;
                status_ = std::string("Saved: ") + target;
            }
            else
            {
                status_ = std::string("Save failed: ") + target;
            }
        }
        // Do not quit on Ctrl+Shift+C; it's used for copy
        const auto &lines = buffer_->lines();
        auto clamp_col_to_line = [&](int y)
        {
            if (y < 1)
                y = 1;
            if (y > (int)lines.size())
                y = (int)lines.size();
            int len = (int)(y >= 1 && y <= (int)lines.size() ? lines[y - 1].size() : 0);
            if (cx_ > len + 1)
                cx_ = len + 1;
        };

        switch (key)
        {
        // While in search mode, use arrows to navigate matches
        case input::KEY_UP:
            if (cy_ > 1)
                cy_--;
            break;
        case input::KEY_DOWN:
            if (cy_ < (int)lines.size())
                cy_++;
            break;
        case input::KEY_CTRL_UP:
            cy_ -= 6;
            if (cy_ < 1)
                cy_ = 1;
            break;
        case input::KEY_CTRL_DOWN:
            cy_ += 6;
            if (cy_ > (int)lines.size())
                cy_ = (int)lines.size();
            break;
        case input::KEY_HOME:
            cx_ = 1;
            break;
        case input::KEY_END:
            if (cy_ >= 1 && cy_ <= (int)lines.size())
                cx_ = (int)lines[cy_ - 1].size() + 1;
            break;
        case input::KEY_CTRL_HOME:
            cy_ = 1;
            cx_ = 1;
            break;
        case input::KEY_CTRL_END:
            cy_ = (int)lines.size();
            cx_ = (cy_ >= 1 ? (int)lines[cy_ - 1].size() + 1 : 1);
            break;
        case input::KEY_LEFT:
            if (cx_ > 1)
            {
                cx_--;
            }
            else if (cy_ > 1)
            {
                cy_--;
                int len = (cy_ >= 1 && cy_ <= (int)lines.size()) ? (int)lines[cy_ - 1].size() : 0;
                cx_ = len + 1;
            }
            break;
        case input::KEY_RIGHT:
        {
            int len = (int)(cy_ >= 1 && cy_ <= (int)lines.size() ? lines[cy_ - 1].size() : 0);
            if (cx_ <= len)
            {
                cx_++;
            }
            else if (cy_ < (int)lines.size())
            {
                cy_++;
                cx_ = 1;
            }
            break;
        }
        case input::KEY_ENTER:
        {
            int row = cy_ - 1;
            int col = cx_ - 1;
            if (row < 0)
                row = 0;
            if (row >= (int)buffer_->line_count())
                row = (int)buffer_->line_count() - 1;
            if (col < 0)
                col = 0;
            buffer_->split_line((size_t)row, (size_t)col);
            cy_++;
            cx_ = 1;
            modified_ = true;
            break;
        }
        case input::KEY_BACKSPACE:
        {
            if (selection_active())
            {
                delete_selection();
                break;
            }
            if (cy_ < 1)
                cy_ = 1;
            if (cy_ > (int)buffer_->line_count())
                cy_ = (int)buffer_->line_count();
            int row = cy_ - 1;
            if (cx_ > 1)
            {
                buffer_->delete_char((size_t)row, (size_t)(cx_ - 2));
                cx_--;
                modified_ = true;
            }
            else if (cy_ > 1)
            {
                int prev_len = (int)buffer_->line_length((size_t)(row - 1));
                buffer_->join_with_next((size_t)(row - 1));
                cy_--;
                cx_ = prev_len + 1;
                modified_ = true;
            }
            break;
        }
        case input::KEY_DELETE:
        {
            // If selection is active, delete it, then also delete the char at the new cursor
            if (selection_active())
            {
                delete_selection();
            }
            if (cy_ < 1)
                cy_ = 1;
            if (cy_ > (int)buffer_->line_count())
                cy_ = (int)buffer_->line_count();
            int row = cy_ - 1;
            int len2 = (int)buffer_->line_length((size_t)row);
            if (cx_ <= len2)
            {
                buffer_->delete_char((size_t)row, (size_t)(cx_ - 1));
                modified_ = true;
            }
            else if (row + 1 < (int)buffer_->line_count())
            {
                buffer_->join_with_next((size_t)row);
                modified_ = true;
            }
            break;
        }
        case input::KEY_CTRL_LEFT:
        {
            int row = cy_ - 1;
            if (row < 0)
                row = 0;
            if (row >= (int)buffer_->line_count())
                row = (int)buffer_->line_count() - 1;
            int col = cx_ - 1;
            const auto &s = buffer_->lines()[row];
            if (col == 0 && row > 0)
            {
                cy_--;
                cx_ = (int)buffer_->line_length((size_t)(row - 1)) + 1;
                break; // go up if on x = 0
            }
            if (col > (int)s.size())
                col = (int)s.size(); // prevent escape from text
            if (col > 0)
                col--;
            auto is_word = [](char ch)
            { return std::isalnum((signed char)ch) || ch == '_'; };
            while (col > 0 && std::isspace((signed char)s[col]))
                col--;
            while (col > 0 && is_word(s[col - 1]))
                col--;
            cx_ = col + 1;
            break;
        }
        case input::KEY_CTRL_RIGHT:
        {
            int row = cy_ - 1;
            if (row < 0)
                row = 0;
            if (row >= (int)buffer_->line_count())
                row = (int)buffer_->line_count() - 1;
            int col = cx_ - 1;
            const auto &s = buffer_->lines()[row];
            // skipp down
            if ((size_t)col == s.size() && cy_ < (int)buffer_->line_count())
            {
                ++cy_;
                cx_ = 1; // Changed from 0 to 1 (1-based positioning)
                break;
            }

            auto is_word = [](char ch)
            { return std::isalnum((signed char)ch) || ch == '_'; };
            int n = (int)s.size();
            // Skip current word/token first
            if (col < n && is_word(s[col]))
            {
                // Currently on a word character, skip to end of word
                while (col < n && is_word(s[col]))
                    col++;
            }
            else if (col < n && !std::isspace((signed char)s[col]))
            {
                // Currently on punctuation, skip to end of punctuation
                while (col < n && !is_word(s[col]) && !std::isspace((signed char)s[col]))
                    col++;
            }

            // Skip any whitespace to get to next token
            while (col < n && std::isspace((signed char)s[col]))
                col++;
            cx_ = col + 1;
            break;
        }
        case input::KEY_PAGE_UP:
        case input::KEY_PAGE_DOWN:
        {
            auto sz = screen_->size();
            int page = sz.rows - 1;
            if (page < 1)
                page = 1;
            if (key == input::KEY_PAGE_UP)
            {
                cy_ -= page;
                if (cy_ < 1)
                    cy_ = 1;
            }
            else
            {
                cy_ += page;
                if (cy_ > (int)lines.size())
                    cy_ = (int)lines.size();
            }
            break;
        }
        case input::KEY_CTRL_K:
        {
            if (cy_ < 1)
                cy_ = 1;
            if (cy_ > (int)buffer_->line_count())
                cy_ = (int)buffer_->line_count();
            int row = cy_ - 1;
            buffer_->delete_line((size_t)row);
            if (cy_ > (int)buffer_->line_count())
                cy_ = (int)buffer_->line_count();
            int len = (int)buffer_->line_length((size_t)(cy_ - 1));
            if (cx_ > len + 1)
                cx_ = len + 1;
            modified_ = true;
            break;
        }
        // Selection via Shift + arrows and Ctrl+Shift + arrows
        case input::KEY_SHIFT_LEFT:
            start_selection_if_needed();
            if (cx_ > 1)
            {
                cx_--;
            }
            else if (cy_ > 1)
            {
                cy_--;
                cx_ = (int)lines[cy_ - 1].size() + 1;
            }
            break;
        case input::KEY_SHIFT_RIGHT:
        {
            start_selection_if_needed();
            int len = (int)(cy_ >= 1 && cy_ <= (int)lines.size() ? lines[cy_ - 1].size() : 0);
            if (cx_ <= len)
            {
                cx_++;
            }
            else if (cy_ < (int)lines.size())
            {
                cy_++;
                cx_ = 1;
            }
            break;
        }
        case input::KEY_SHIFT_UP:
            start_selection_if_needed();
            if (cy_ > 1)
                cy_--;
            break;
        case input::KEY_SHIFT_DOWN:
            start_selection_if_needed();
            if (cy_ < (int)lines.size())
                cy_++;
            break;
        case input::KEY_CTRL_SHIFT_LEFT:
        {
            start_selection_if_needed();
            int row = cy_ - 1;
            if (row < 0)
                row = 0;
            int col = cx_ - 1;
            const auto &s = buffer_->lines()[row];
            auto is_word = [](char ch)
            { return std::isalnum((unsigned char)ch) || ch == '_'; };
            auto is_space = [](char ch)
            { return std::isspace((unsigned char)ch); };
            auto is_punct = [&](char ch)
            { return !is_word(ch) && !is_space(ch); };
            if (col > (int)s.size())
                col = (int)s.size();
            if (col > 0)
                col--; // start from previous
            while (col > 0 && is_space(s[col]))
                col--; // skip spaces
            if (col >= 0 && is_punct(s[col]))
            {
                while (col > 0 && is_punct(s[col - 1]))
                    col--;
            }
            else
            {
                while (col > 0 && is_word(s[col - 1]))
                    col--; // start of word
            }
            cx_ = col + 1;
            break;
        }
        case input::KEY_CTRL_SHIFT_RIGHT:
        {
            start_selection_if_needed();
            int row = cy_ - 1;
            if (row < 0)
                row = 0;
            int col = cx_ - 1;
            const auto &s = buffer_->lines()[row];
            auto is_word = [](char ch)
            { return std::isalnum((unsigned char)ch) || ch == '_'; };
            auto is_space = [](char ch)
            { return std::isspace((unsigned char)ch); };
            auto is_punct = [&](char ch)
            { return !is_word(ch) && !is_space(ch); };
            int n = (int)s.size();
            if (col < n)
            {
                if (is_word(s[col]))
                {
                    while (col < n && is_word(s[col]))
                        col++;
                }
                else if (is_punct(s[col]))
                {
                    while (col < n && is_punct(s[col]))
                        col++;
                }
                while (col < n && is_space(s[col]))
                    col++;
            }
            cx_ = col + 1;
            break;
        }
        case input::KEY_CTRL_G:
        {

            debug_note("Ctrl-G");
            // add Pressed KEy_CTRL_G to status on the left

            // Jump to closest parenthesis on the right of the cursor in current line
            int col = cx_ - 1;
            int row = cy_ - 1;
            const auto &s = buffer_->lines()[row];

            size_t parenthesis_open_ct = 0;
            auto is_parenthesis = [&parenthesis_open_ct](char ch)
            {
                if (ch == '(')
                    parenthesis_open_ct++;
                return (ch == ')' && parenthesis_open_ct >= 1);
            };
            int n = (int)s.size();
            while (col < n && !is_parenthesis(s[col]))
                col++;
            cx_ = col + 1;
            break;

        } // add new cases from here on: ....
        case input::KEY_CTRL_SHIFT_UP:
            start_selection_if_needed();
            cy_ -= 6;
            if (cy_ < 1)
                cy_ = 1;
            break;
        case input::KEY_CTRL_SHIFT_DOWN:
            start_selection_if_needed();
            cy_ += 6;
            if (cy_ > (int)lines.size())
                cy_ = (int)lines.size();
            break;
        default:
            // Insert printable ASCII and tab
            if ((key >= 32 && key <= 126) || key == '\t')
            {
                if (selection_active())
                {
                    delete_selection();
                }
                int row = cy_ - 1;
                int col = cx_ - 1;
                if (row < 0)
                    row = 0;
                if (row >= (int)buffer_->line_count())
                    row = (int)buffer_->line_count() - 1;
                if (col < 0)
                    col = 0;
                buffer_->insert_char((size_t)row, (size_t)col, static_cast<char>(key));
                cx_++;
                modified_ = true;
            }
            break;
        }
        // Keep selection for all navigation keys to tolerate terminals that
        // sometimes drop Shift modifiers mid-press. Clear on non-nav actions.
        auto is_nav_key = [&](int k)
        {
            switch (k)
            {
            case input::KEY_LEFT:
            case input::KEY_RIGHT:
            case input::KEY_UP:
            case input::KEY_DOWN:
            case input::KEY_HOME:
            case input::KEY_END:
            case input::KEY_CTRL_LEFT:
            case input::KEY_CTRL_RIGHT:
            case input::KEY_CTRL_UP:
            case input::KEY_CTRL_DOWN:
            case input::KEY_CTRL_HOME:
            case input::KEY_CTRL_END:
            case input::KEY_PAGE_UP:
            case input::KEY_PAGE_DOWN:
            case input::KEY_SHIFT_LEFT:
            case input::KEY_SHIFT_RIGHT:
            case input::KEY_SHIFT_UP:
            case input::KEY_SHIFT_DOWN:
            case input::KEY_CTRL_SHIFT_LEFT:
            case input::KEY_CTRL_SHIFT_RIGHT:
            case input::KEY_CTRL_SHIFT_UP:
            case input::KEY_CTRL_SHIFT_DOWN:
            case input::KEY_UNKNOWN: // partial sequences; don't drop selection
                return true;
            default:
                return false;
            }
        };
        if (!is_nav_key(key))
            selecting_ = false;
        clamp_col_to_line(cy_);
        scroll();
        return true;
    }

    void Editor::start_search()
    {
        searching_ = true;
        std::string query = search_query_;
        status_.clear();
        while (true)
        {
            screen_->draw_status(std::string("Search: ") + query + (search_matches_.empty() ? "" : ("  [" + std::to_string(search_matches_.size()) + " matches]")));
            screen_->flush();
            int k = input::read_key();
            if (k == input::KEY_ENTER)
            {
                search_query_ = query;
                update_search_matches();
                if (!search_matches_.empty())
                {
                    search_index_ = 0;
                    jump_to_match(search_index_);
                }
                searching_ = false;
                status_.clear();
                break;
            }
            if (k == 27) // ESC quit search
            {
                searching_ = false;
                status_ = "Search canceled";
                break;
            }
            if (k == input::KEY_BACKSPACE)
            {
                if (!query.empty())
                    query.pop_back(); // TODO does'nt use cursor
                search_query_ = query;
                update_search_matches();
                if (!search_matches_.empty())
                {
                    search_index_ = 0;
                    jump_to_match(search_index_);
                }
                continue;
            }
            if (k == input::KEY_UP)
            {
                if (!search_matches_.empty())
                {
                    int n = (int)search_matches_.size();
                    if (search_index_ < 0)
                        search_index_ = 0;
                    search_index_ = (search_index_ - 1 + n) % n; // Make sure n is always in range
                    jump_to_match(search_index_);
                }
                continue;
            }
            if (k == input::KEY_DOWN)
            {
                if (!search_matches_.empty())
                {
                    int n = (int)search_matches_.size();
                    if (search_index_ < 0)
                        search_index_ = 0;
                    search_index_ = (search_index_ + 1) % n;
                    jump_to_match(search_index_);
                }
                continue;
            }
            if (k >= 32 && k <= 126) // no utf8 characters
            {
                query.push_back(static_cast<char>(k));
                search_query_ = query;
                update_search_matches();
                if (!search_matches_.empty())
                {
                    search_index_ = 0;
                    jump_to_match(search_index_);
                }
            }
        }
    }

    void Editor::copy_selection_to_clipboard()
    {
        if (!selection_active())
            return;
        int aL = anchor_cy_ - 1, aC = anchor_cx_ - 1;
        int cL = cy_ - 1, cC = cx_ - 1;
        if (aL > cL || (aL == cL && aC > cC))
        {
            std::swap(aL, cL);
            std::swap(aC, cC);
        }
        aL = std::clamp(aL, 0, (int)buffer_->line_count() - 1);
        cL = std::clamp(cL, 0, (int)buffer_->line_count() - 1);
        aC = std::clamp(aC, 0, (int)buffer_->line_length((size_t)aL));
        cC = std::clamp(cC, 0, (int)buffer_->line_length((size_t)cL));
        std::string out;
        if (aL == cL)
        {
            const auto &s = buffer_->lines()[aL];
            if (aC < cC)
                out.assign(s.begin() + aC, s.begin() + cC);
            else
                out.clear();
        }
        else
        {
            // First line tail
            const auto &s0 = buffer_->lines()[aL];
            out.assign(s0.begin() + aC, s0.end());
            out.push_back('\n');
            // Middle full lines
            for (int li = aL + 1; li < cL; ++li)
            {
                out.append(buffer_->lines()[li]);
                out.push_back('\n');
            }
            // Last line head
            const auto &sl = buffer_->lines()[cL];
            out.append(sl.begin(), sl.begin() + cC);
        }
        clipboard_ = std::move(out);
        status_ = "Copied";
    }

    void Editor::paste_from_clipboard()
    {
        if (clipboard_.empty())
        {
            status_ = "Clipboard empty";
            return;
        }
        // If selection active, replace it
        if (selection_active())
            delete_selection();
        int row = cy_ - 1;
        int col = cx_ - 1;
        if (row < 0)
            row = 0;
        if (row >= (int)buffer_->line_count())
            row = (int)buffer_->line_count() - 1;
        if (col < 0)
            col = 0;
        for (char ch : clipboard_)
        {
            if (ch == '\n')
            {
                buffer_->split_line((size_t)row, (size_t)col);
                row += 1;
                col = 0;
                cy_ = row + 1;
                cx_ = col + 1;
            }
            else
            {
                buffer_->insert_char((size_t)row, (size_t)col, ch);
                col += 1;
                cx_ = col + 1;
            }
        }
        modified_ = true;
        scroll();
    }

    void Editor::debug_note(const std::string &note)
    {
        last_key_info_ = note;
        // Do not clear status_; render will append last_key_info_ after status
    }

    void Editor::update_search_matches()
    {
        search_matches_.clear();
        search_index_ = -1;
        if (search_query_.empty())
            return;
        for (int li = 0; li < (int)buffer_->line_count(); ++li)
        { // loop through every line
            const auto &s = buffer_->lines()[li];
            size_t pos = 0;
            while (true)
            {
                pos = s.find(search_query_, pos); // returns npos if not available
                if (pos == std::string::npos)
                    break; // npos -> largest value
                search_matches_.push_back(Match{li, (int)pos, (int)(pos + search_query_.size())});
                pos += (pos < s.size() ? 1 : 0);
            }
        }
    }

    void Editor::jump_to_match(int index)
    {
        if (index < 0 || index >= (int)search_matches_.size())
            return;
        const auto &m = search_matches_[index];
        cy_ = m.line + 1;
        cx_ = m.start + 1;
        // clamp column within line bounds
        if (cy_ < 1)
            cy_ = 1;
        if (cy_ > (int)buffer_->line_count())
            cy_ = (int)buffer_->line_count();
        int len = (int)buffer_->line_length((size_t)(cy_ - 1));
        if (cx_ < 1)
            cx_ = 1;
        if (cx_ > len + 1)
            cx_ = len + 1;
        scroll();
    }

} // namespace termite
