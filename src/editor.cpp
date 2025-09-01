#include "termite/editor.hpp"

#include "termite/screen.hpp"
#include "termite/buffer.hpp"
#include "termite/input.hpp"
#include "termite/file_io.hpp"
#include "termite/platform.hpp"
#include "termite/ansi.hpp"

#include <iostream>
#include <cctype>
#include <algorithm>
#include <string_view>
#include <cctype>

namespace termite
{

    Editor::Editor()
        : screen_(new Screen()), buffer_(new Buffer())
    {
    }

    void Editor::delete_selection()
    {
        if (!selection_active()) return;
        // Normalize selection
        int aL = anchor_cy_ - 1, aC = anchor_cx_ - 1;
        int cL = cy_ - 1, cC = cx_ - 1;
        if (aL > cL || (aL == cL && aC > cC)) { std::swap(aL, cL); std::swap(aC, cC); }
        int line_count = (int)buffer_->line_count();
        if (line_count <= 0) { selecting_ = false; return; }
        aL = std::clamp(aL, 0, line_count - 1);
        cL = std::clamp(cL, 0, line_count - 1);
        aC = std::clamp(aC, 0, (int)buffer_->line_length((size_t)aL));
        cC = std::clamp(cC, 0, (int)buffer_->line_length((size_t)cL));

        if (aL == cL) {
            // Delete [aC, cC) on the same line
            int count = cC - aC;
            for (int i = 0; i < count; ++i) {
                buffer_->delete_char((size_t)aL, (size_t)aC);
            }
        } else {
            // Trim end line prefix [0, cC)
            for (int i = 0; i < cC; ++i) {
                buffer_->delete_char((size_t)cL, 0);
            }
            // Trim start line suffix from aC to end
            while ((int)buffer_->line_length((size_t)aL) > aC) {
                buffer_->delete_char((size_t)aL, (size_t)aC);
            }
            // Join lines from aL with next until cL collapses
            while (aL + 1 <= cL) {
                buffer_->join_with_next((size_t)aL);
                --cL;
            }
        }
        cy_ = aL + 1;
        cx_ = aC + 1;
        selecting_ = false;
        modified_ = true;
    }

    std::string Editor::prompt_input(const std::string &prompt, const std::string &initial)
    {
        std::string input = initial;
        while (true)
        {
            screen_->draw_status(prompt + input);
            screen_->flush();
            int k = input::read_key();
            if (k == input::KEY_ENTER) {
                return input;
            } else if (k == input::KEY_CTRL_C || k == 27 /*ESC*/) {
                return std::string();
            } else if (k == input::KEY_BACKSPACE) {
                if (!input.empty()) input.pop_back();
            } else if (k >= 32 && k <= 126) {
                input.push_back(static_cast<char>(k));
            }
        }
    }

    Editor::~Editor()
    {
        platform::shutdown();
    }

    int Editor::run(int argc, char **argv)
    {
        status_ = "Termite — Press Ctrl-Q to quit";
        if (!platform::init())
        { // initialise terminal raw mode
            std::cerr << "Failed to initialize terminal (raw mode).\n";
        }
        open_file_if_provided(argc, argv);

        // Basic loop: render and read keys
        while (true)
        {
            render();

            int k = input::read_key();
            if (!handle_input(k))
                break;
        }
        return 0;
    }

    void Editor::open_file_if_provided(int argc, char **argv)
    {
        if (argc > 1 && argv[1] && argv[1][0] != '\0')
        {
            try
            {
                auto contents = file_io::read_file(argv[1]); // read firstparameter file
                // move resource to buffer (more performant than copying)
                buffer_->set_contents(std::move(contents));
                status_ = std::string("Opened: ") + argv[1];
                filename_ = argv[1];
                modified_ = false;
            }
            catch (...)
            {
                status_ = std::string("Failed to open: ") + argv[1];
            }
        }
    }

    void Editor::render()
    {
        screen_->clear();
        // Draw file lines with vertical + horizontal scrolling + selection highlight
        auto sz = screen_->size();
        int max_rows = sz.rows - 1; // leave one line for status
        int max_cols = sz.cols;
        const auto &lines = buffer_->lines();
        auto disp_col = [](const std::string& s, int char_col, int tabw) {
            if (char_col < 0) char_col = 0;
            if (char_col > (int)s.size()) char_col = (int)s.size();
            int col = 0;
            for (int i = 0; i < char_col; ++i) {
                if (s[i] == '\t') {
                    int spaces = tabw - (col % tabw);
                    col += spaces;
                } else {
                    col += 1;
                }
            }
            return col;
        };
        auto expand_tabs = [](const std::string& s, int tabw) {
            std::string out;
            out.reserve(s.size());
            int col = 0;
            for (char ch : s) {
                if (ch == '\t') {
                    int spaces = tabw - (col % tabw);
                    out.append(spaces, ' ');
                    col += spaces;
                } else {
                    out.push_back(ch);
                    col += 1;
                }
            }
            return out;
        };
        constexpr int TAB_W = 4;
        for (int i = 0; i < max_rows; ++i)
        {
            int file_row = row_off_ + i; // 0-based add scrolling
            screen_->move_cursor(i + 1, 1);
            if (file_row < (int)lines.size())
            {
                const std::string &line = lines[file_row];
                std::string rendered = expand_tabs(line, TAB_W);
                if (col_off_ < (int)rendered.size())
                {
                    std::string_view vis(rendered.c_str() + col_off_, rendered.size() - col_off_);
                    if ((int)vis.size() > max_cols) vis = vis.substr(0, max_cols);

                    bool has_sel = selecting_;
                    if (has_sel) {
                        int aL = anchor_cy_ - 1, aC = anchor_cx_ - 1;
                        int cL = cy_ - 1, cC = cx_ - 1;
                        if (aL > cL || (aL == cL && aC > cC)) { std::swap(aL, cL); std::swap(aC, cC); }
                        if (file_row < aL || file_row > cL) {
                            has_sel = false;
                        } else {
                            int line_len = (int)line.size();
                            int s = 0, e = line_len; // selection [s,e) on this line (char indices)
                            if (aL == cL) { s = std::clamp(aC, 0, line_len); e = std::clamp(cC, 0, line_len); }
                            else if (file_row == aL) { s = std::clamp(aC, 0, line_len); e = line_len; }
                            else if (file_row == cL) { s = 0; e = std::clamp(cC, 0, line_len); }
                            else { s = 0; e = line_len; }

                            int s_disp = disp_col(line, s, TAB_W);
                            int e_disp = disp_col(line, e, TAB_W);
                            int vis_start = col_off_;
                            int vis_end = col_off_ + (int)vis.size();
                            int hs = std::clamp(s_disp, vis_start, vis_end);
                            int he = std::clamp(e_disp, vis_start, vis_end);
                            int before = hs - vis_start;
                            int hlen = he - hs;
                            if (before > 0) screen_->write(std::string(vis.substr(0, before)));
                            if (hlen > 0) {
                                screen_->write(ansi::REVERSE);
                                screen_->write(std::string(vis.substr(before, hlen)));
                                screen_->write(ansi::RESET);
                            }
                            int after_start = before + std::max(0, hlen);
                            if (after_start < (int)vis.size()) screen_->write(std::string(vis.substr(after_start)));
                            continue;
                        }
                    }
                    screen_->write(std::string(vis));
                }
            }
        }

        // Status with filename, position, and modified marker
        std::string fname = filename_.empty() ? std::string("(untitled)") : filename_;
        std::string mod = modified_ ? " *" : "";
        std::string pos = " Ln " + std::to_string(cy_) + ", Col " + std::to_string(cx_);
        std::string msg = status_.empty() ? std::string("") : std::string(" | ") + status_;
        screen_->draw_status(fname + mod + " |" + pos + msg);

        // Place cursor at current position within viewport (tab-aware)
        int screen_row = cy_ - row_off_;
        if (screen_row < 1)
            screen_row = 1;
        if (screen_row > max_rows)
            screen_row = max_rows;
        int screen_col = 1;
        if (cy_ >= 1 && cy_ <= (int)lines.size()) {
            const std::string& line = lines[cy_-1];
            int c_disp = disp_col(line, cx_ - 1, TAB_W);
            screen_col = c_disp - col_off_ + 1;
        }
        if (screen_col < 1)
            screen_col = 1;
        if (screen_col > max_cols)
            screen_col = max_cols;
        screen_->move_cursor(screen_row, screen_col);
        screen_->flush();
    }

    bool Editor::handle_input(int key)
    {
        if (key == input::KEY_CTRL_Q)
        {
            return false;
        }
        if (key == input::KEY_CTRL_S)
        {
            std::string target = filename_.empty() ? std::string("") : filename_;
            if (target.empty()) {
                std::string entered = prompt_input("Save As: ", filename_);
                if (entered.empty()) {
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
        if (key == input::KEY_CTRL_C)
        {
            return false;
        }
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
        case input::KEY_UP:
            if (cy_ > 1)
                cy_--;
            break;
        case input::KEY_DOWN:
            if (cy_ < (int)lines.size())
                cy_++;
            break;
        case input::KEY_CTRL_UP:
            cy_ -= 6; if (cy_ < 1) cy_ = 1; break;
        case input::KEY_CTRL_DOWN:
            cy_ += 6; if (cy_ > (int)lines.size()) cy_ = (int)lines.size(); break;
        case input::KEY_HOME:
            cx_ = 1;
            break;
        case input::KEY_END:
            if (cy_ >= 1 && cy_ <= (int)lines.size())
                cx_ = (int)lines[cy_ - 1].size() + 1;
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
            if (selection_active()) { delete_selection(); break; }
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
            if (selection_active()) { delete_selection(); break; }
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
        // Selection via Shift + arrows and Ctrl+Shift + arrows
        case input::KEY_SHIFT_LEFT:
            start_selection_if_needed();
            if (cx_ > 1) { cx_--; }
            else if (cy_ > 1) { cy_--; cx_ = (int)lines[cy_-1].size() + 1; }
            break;
        case input::KEY_SHIFT_RIGHT:
        {
            start_selection_if_needed();
            int len = (int)(cy_ >= 1 && cy_ <= (int)lines.size() ? lines[cy_-1].size() : 0);
            if (cx_ <= len) { cx_++; }
            else if (cy_ < (int)lines.size()) { cy_++; cx_ = 1; }
            break;
        }
        case input::KEY_SHIFT_UP:
            start_selection_if_needed(); if (cy_ > 1) cy_--; break;
        case input::KEY_SHIFT_DOWN:
            start_selection_if_needed(); if (cy_ < (int)lines.size()) cy_++; break;
        case input::KEY_CTRL_SHIFT_LEFT:
        {
            start_selection_if_needed();
            int row = cy_ - 1; if (row < 0) row = 0; int col = cx_ - 1;
            const auto& s = buffer_->lines()[row];
            auto is_word = [](char ch){ return std::isalnum((unsigned char)ch) || ch == '_'; };
            if (col > (int)s.size()) col = (int)s.size(); if (col > 0) col--;
            while (col > 0 && std::isspace((unsigned char)s[col])) col--;
            while (col > 0 && is_word(s[col-1])) col--;
            cx_ = col + 1; break;
        }
        case input::KEY_CTRL_SHIFT_RIGHT:
        {
            start_selection_if_needed();
            int row = cy_ - 1; if (row < 0) row = 0; int col = cx_ - 1;
            const auto& s = buffer_->lines()[row];
            auto is_word = [](char ch){ return std::isalnum((unsigned char)ch) || ch == '_'; };
            int n = (int)s.size();
            while (col < n && is_word(s[col])) col++;
            while (col < n && std::isspace((unsigned char)s[col])) col++;
            cx_ = col + 1; break;
        }
        case input::KEY_CTRL_SHIFT_UP:
            start_selection_if_needed(); cy_ -= 6; if (cy_ < 1) cy_ = 1; break;
        case input::KEY_CTRL_SHIFT_DOWN:
            start_selection_if_needed(); cy_ += 6; if (cy_ > (int)lines.size()) cy_ = (int)lines.size(); break;
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
            if (cy_ < 1) cy_ = 1;
            if (cy_ > (int)buffer_->line_count()) cy_ = (int)buffer_->line_count();
            int row = cy_ - 1;
            buffer_->delete_line((size_t)row);
            if (cy_ > (int)buffer_->line_count()) cy_ = (int)buffer_->line_count();
            int len = (int)buffer_->line_length((size_t)(cy_-1));
            if (cx_ > len + 1) cx_ = len + 1;
            modified_ = true;
            break;
        }
        case input::KEY_F2: // Save As prompt
        {
            std::string entered = prompt_input("Save As: ", filename_);
            if (!entered.empty()) {
                if (file_io::save_file(*buffer_, entered)) {
                    filename_ = entered;
                    modified_ = false;
                    status_ = std::string("Saved: ") + entered;
                } else {
                    status_ = std::string("Save failed: ") + entered;
                }
            } else {
                status_ = "Save canceled";
            }
            break;
        }
        default:
            // Insert printable ASCII and tab
            if ((key >= 32 && key <= 126) || key == '\t')
            {
                if (selection_active()) { delete_selection(); }
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
        // Clear selection when the last key was not a shift-extended navigation
        switch (key) {
            case input::KEY_SHIFT_LEFT: case input::KEY_SHIFT_RIGHT:
            case input::KEY_SHIFT_UP: case input::KEY_SHIFT_DOWN:
            case input::KEY_CTRL_SHIFT_LEFT: case input::KEY_CTRL_SHIFT_RIGHT:
            case input::KEY_CTRL_SHIFT_UP: case input::KEY_CTRL_SHIFT_DOWN:
                break; // keep selection
            default:
                selecting_ = false; // clear selection
                break;
        }
        clamp_col_to_line(cy_);
        scroll();
        return true;
    }

    void Editor::scroll()
    {
        const auto &lines = buffer_->lines();
        int max_line_rows = (int)lines.size();
        int line_index = (cy_ >= 1 && cy_ <= max_line_rows) ? (cy_ - 1) : -1;
        auto disp_col = [](const std::string& s, int char_col, int tabw) {
            if (char_col < 0) char_col = 0;
            if (char_col > (int)s.size()) char_col = (int)s.size();
            int col = 0;
            for (int i = 0; i < char_col; ++i) {
                if (s[i] == '\t') { int spaces = tabw - (col % tabw); col += spaces; }
                else { col += 1; }
            }
            return col;
        };
        auto disp_len = [&](const std::string& s) {
            return disp_col(s, (int)s.size(), 4);
        };

        auto sz = screen_->size();
        int max_rows = sz.rows - 1; // minus status line
        if (cy_ < row_off_ + 1)
        {
            row_off_ = cy_ - 1;
            if (row_off_ < 0)
                row_off_ = 0;
        }
        else if (cy_ > row_off_ + max_rows)
        {
            row_off_ = cy_ - max_rows;
            if (row_off_ < 0)
                row_off_ = 0;
            if (cy_ > max_line_rows)
            {
                row_off_ = std::max(0, max_line_rows - max_rows);
            }
        }

        int max_cols = sz.cols;
        if (line_index >= 0) {
            const std::string& line = lines[line_index];
            int c_disp = disp_col(line, cx_ - 1, 4);
            int line_disp_len = disp_len(line);
            if (c_disp < col_off_) {
                col_off_ = c_disp;
                if (col_off_ < 0) col_off_ = 0;
            } else if (c_disp >= col_off_ + max_cols) {
                col_off_ = c_disp - max_cols + 1;
                if (col_off_ < 0) col_off_ = 0;
            }
            int max_off = std::max(0, line_disp_len - max_cols);
            if (col_off_ > max_off) col_off_ = max_off;
        }
    }

} // namespace termite
