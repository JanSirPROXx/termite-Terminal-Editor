#include "termite/editor.hpp"

#include "termite/screen.hpp"
#include "termite/buffer.hpp"
#include "termite/ansi.hpp"

#include <algorithm>
#include <string>
#include <string_view>
#ifndef _WIN32
#include <sys/stat.h>
#endif

namespace termite {

void Editor::render()
{
    screen_->clear();
    auto perms_for = [&](const std::string& path) -> std::string {
#ifndef _WIN32
        if (path.empty()) return std::string("(no-file)");
        struct stat st{};
        if (stat(path.c_str(), &st) != 0) return std::string("(stat-failed)");
        auto m = st.st_mode;
        auto bit = [&](mode_t b, char c) { return (m & b) ? c : '-'; };
        char t = S_ISDIR(m) ? 'd' : S_ISCHR(m) ? 'c' : S_ISBLK(m) ? 'b' : S_ISFIFO(m) ? 'p' : S_ISSOCK(m) ? 's' : '-';
        std::string s;
        s.reserve(11);
        s.push_back(t);
        s.push_back(bit(S_IRUSR,'r')); s.push_back(bit(S_IWUSR,'w')); s.push_back(bit(S_IXUSR,'x'));
        s.push_back(bit(S_IRGRP,'r')); s.push_back(bit(S_IWGRP,'w')); s.push_back(bit(S_IXGRP,'x'));
        s.push_back(bit(S_IROTH,'r')); s.push_back(bit(S_IWOTH,'w')); s.push_back(bit(S_IXOTH,'x'));
        return s;
#else
        (void)path; return std::string("(perms N/A)");
#endif
    };

    auto sz = screen_->size();
    int header_rows = 1;
    int status_rows = 1;
    int max_rows = sz.rows - header_rows - status_rows;
    if (max_rows < 1) max_rows = 1;
    // Draw header line
    screen_->move_cursor(1, 1);
#ifdef TERMITE_VERSION
    std::string version = TERMITE_VERSION;
#else
    std::string version = "0.1";
#endif
    std::string fname_hdr = filename_.empty() ? std::string("(untitled)") : filename_;
    std::string header = std::string("Termite by Jan v") + version + " | " + fname_hdr + " " + perms_for(filename_);
    int cols = sz.cols;
    if ((int)header.size() > cols) header.resize(cols);
    int pad_left = 0;
    if (cols > 0) pad_left = std::max(0, (cols - (int)header.size()) / 2);
    std::string style = ansi::bg_color256(15) + ansi::color256(18) + ansi::BOLD;
    screen_->write(style);
    screen_->write(std::string(cols, ' '));
    screen_->move_cursor(1, pad_left + 1);
    screen_->write(header);
    screen_->write(ansi::RESET);

    int max_cols = sz.cols;
    const auto &lines = buffer_->lines();
    auto digits = [](int n){ int d = 1; while (n >= 10) { n /= 10; ++d; } return d; }; //calculate nr of digits in pos int to allocate mem
    int lnw = std::max(4, digits((int)lines.size())); // gutter width (at least 4)
    int prefix_cols = lnw + 2; // number + space + '|'
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
        screen_->move_cursor(i + 1 + header_rows, 1);
        // Draw line number gutter
        if (file_row < (int)lines.size()) {
            int lineno = file_row + 1;
            std::string num = std::to_string(lineno);
            if ((int)num.size() < lnw) num.insert(num.begin(), lnw - (int)num.size(), ' ');
            screen_->write(ansi::color256(57)); //Color for row nr's grey: 245
            screen_->write(num);
            screen_->write(" ");
            screen_->write(ansi::RESET);
            screen_->write("|");

            const std::string &line = lines[file_row];
            int text_cols = std::max(0, max_cols - prefix_cols);
            screen_->move_cursor(i + 1 + header_rows, lnw + 3);
            std::string rendered = expand_tabs(line, TAB_W);
            if (text_cols > 0 && col_off_ < (int)rendered.size()) {
                //string_view to render
                std::string_view vis(rendered.c_str() + col_off_, rendered.size() - col_off_);
                if ((int)vis.size() > text_cols) vis = vis.substr(0, text_cols);

                bool has_sel = selecting_; //selection/ highlighting bool
                if (has_sel) {
                    int aL = anchor_cy_ - 1, aC = anchor_cx_ - 1;
                    int cL = cy_ - 1, cC = cx_ - 1;
                    if (aL > cL || (aL == cL && aC > cC)) { std::swap(aL, cL); std::swap(aC, cC); }
                    if (file_row < aL || file_row > cL) {
                        has_sel = false;
                    } else {
                        int line_len = (int)line.size();
                        int s = 0, e = line_len;
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

                if (!has_sel && !search_query_.empty() && !search_matches_.empty()) {
                    std::vector<std::pair<int,int>> spans;
                    for (const auto& m : search_matches_) {
                        if (m.line != file_row) continue;
                        int s_disp = disp_col(line, m.start, TAB_W);
                        int e_disp = disp_col(line, m.end, TAB_W);
                        int vis_start = col_off_;
                        int vis_end = col_off_ + (int)vis.size();
                        int hs = std::clamp(s_disp, vis_start, vis_end);
                        int he = std::clamp(e_disp, vis_start, vis_end);
                        if (he > hs) spans.emplace_back(hs - vis_start, he - vis_start);
                    }
                    std::sort(spans.begin(), spans.end());
                    std::vector<std::pair<int,int>> merged;
                    for (auto& sp : spans) {
                        if (merged.empty() || sp.first > merged.back().second) merged.push_back(sp);
                        else merged.back().second = std::max(merged.back().second, sp.second);
                    }
                    int pos = 0;
                    for (auto& sp : merged) {
                        if (sp.first > pos) screen_->write(std::string(vis.substr(pos, sp.first - pos)));
                        screen_->write(ansi::bg_color256(11)); //yellow
                        screen_->write(std::string(vis.substr(sp.first, sp.second - sp.first)));
                        screen_->write(ansi::RESET);
                        pos = sp.second;
                    }
                    if (pos < (int)vis.size()) screen_->write(std::string(vis.substr(pos)));
                } else {
                    screen_->write(ansi::color256(51));
                    screen_->write(std::string(vis));
                    screen_->write(ansi::RESET);
                }
            }
        } else {
            // Past EOF: draw empty gutter to keep layout consistent
            screen_->write(std::string(lnw, ' '));
            screen_->write(" |");
        }
    }

    std::string fname = filename_.empty() ? std::string("(untitled)") : filename_;
    std::string mod = modified_ ? " *" : "";
    std::string pos = " Ln " + std::to_string(cy_) + ", Col " + std::to_string(cx_);
    std::string msg;
    if (!status_.empty()) msg += std::string(" | ") + status_;
    if (!last_key_info_.empty()) msg += std::string(" | last: ") + last_key_info_;
    screen_->draw_status(fname + mod + " |" + pos + msg);

    int screen_row = header_rows + (cy_ - row_off_);
    if (screen_row < 1 + header_rows)
        screen_row = 1 + header_rows;
    if (screen_row > header_rows + max_rows)
        screen_row = header_rows + max_rows;
    int screen_col = lnw + 3; // start of content area
    if (cy_ >= 1 && cy_ <= (int)lines.size())
    {
        const std::string &line = lines[cy_ - 1];
        int c_disp = disp_col(line, cx_ - 1, TAB_W);
        screen_col = lnw + 3 + (c_disp - col_off_);
    }
    if (screen_col < 1)
        screen_col = 1;
    if (screen_col > max_cols)
        screen_col = max_cols;
    screen_->move_cursor(screen_row, screen_col);
    screen_->flush();
}

}
