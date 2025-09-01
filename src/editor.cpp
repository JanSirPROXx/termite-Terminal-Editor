// Core editor setup and plumbing
#include "termite/editor.hpp"
#include "termite/screen.hpp"
#include "termite/buffer.hpp"
#include "termite/input.hpp"
#include "termite/file_io.hpp"
#include "termite/platform.hpp"

#include <iostream>
#include <algorithm>

namespace termite {

Editor::Editor() : screen_(new Screen()), buffer_(new Buffer()) {}

Editor::~Editor() { platform::shutdown(); }

int Editor::run(int argc, char** argv) {
    status_ = "Termite — Press Ctrl-Q to quit";
    if (!platform::init()) {
        std::cerr << "Failed to initialize terminal (raw mode).\n";
    }
    open_file_if_provided(argc, argv);
    while (true) {
        render();
        int k = input::read_key();
        if (!handle_input(k)) break;
    }
    return 0;
}

void Editor::open_file_if_provided(int argc, char** argv) {
    if (argc > 1 && argv[1] && argv[1][0] != '\0') {
        try {
            auto contents = file_io::read_file(argv[1]);
            buffer_->set_contents(std::move(contents));
            status_ = std::string("Opened: ") + argv[1];
            filename_ = argv[1];
            modified_ = false;
        } catch (...) {
            status_ = std::string("Failed to open: ") + argv[1];
        }
    }
}

void Editor::scroll() {
    const auto& lines = buffer_->lines();
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
    auto disp_len = [&](const std::string& s) { return disp_col(s, (int)s.size(), 4); };

    auto sz = screen_->size();
    int header_rows = 1;
    int status_rows = 1;
    int max_rows = sz.rows - header_rows - status_rows;
    if (max_rows < 1) max_rows = 1;
    if (cy_ < row_off_ + 1) {
        row_off_ = cy_ - 1;
        if (row_off_ < 0) row_off_ = 0;
    } else if (cy_ > row_off_ + max_rows) {
        row_off_ = cy_ - max_rows;
        if (row_off_ < 0) row_off_ = 0;
        if (cy_ > max_line_rows) {
            row_off_ = std::max(0, max_line_rows - max_rows);
        }
    }

    int max_cols = sz.cols;
    auto digits = [](int n){ int d = 1; while (n >= 10) { n /= 10; ++d; } return d; };
    int lnw = std::max(4, digits((int)lines.size()));
    int text_cols = std::max(1, max_cols - (lnw + 2));
    if (line_index >= 0) {
        const std::string& line = lines[line_index];
        int c_disp = disp_col(line, cx_ - 1, 4);
        int line_disp_len = disp_len(line);
        if (c_disp < col_off_) {
            col_off_ = c_disp;
            if (col_off_ < 0) col_off_ = 0;
        } else if (c_disp >= col_off_ + text_cols) {
            col_off_ = c_disp - text_cols + 1;
            if (col_off_ < 0) col_off_ = 0;
        }
        int max_off = std::max(0, line_disp_len - text_cols);
        if (col_off_ > max_off) col_off_ = max_off;
    }
}

} // namespace termite

