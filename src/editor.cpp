#include "termite/editor.hpp"

#include "termite/screen.hpp"
#include "termite/buffer.hpp"
#include "termite/input.hpp"
#include "termite/file_io.hpp"
#include "termite/platform.hpp"

#include <iostream>

namespace termite {

Editor::Editor()
    : screen_(new Screen()), buffer_(new Buffer()) {
}

Editor::~Editor() {
    platform::shutdown();
}

int Editor::run(int argc, char** argv) {
    status_ = "Termite — Press Ctrl-Q to quit";
    if (!platform::init()) { //initialise terminal raw mode
        std::cerr << "Failed to initialize terminal (raw mode).\n";
    }
    open_file_if_provided(argc, argv);

    // Basic loop: render and read keys
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
            auto contents = file_io::read_file(argv[1]); //read firstparameter file
            //move resource to buffer (more performant than copying)
            buffer_->set_contents(std::move(contents));
            status_ = std::string("Opened: ") + argv[1];
        } catch (...) {
            status_ = std::string("Failed to open: ") + argv[1];
        }
    }
}

void Editor::render() {
    screen_->clear();
    // Draw file lines with simple vertical scrolling
    auto sz = screen_->size();
    int max_rows = sz.rows - 1; // leave one line for status
    const auto& lines = buffer_->lines();
    for (int i = 0; i < max_rows; ++i) {
        int file_row = row_off_ + i; // 0-based
        screen_->move_cursor(i + 1, 1);
        if (file_row < (int)lines.size()) {
            screen_->write(lines[file_row]);
        }
    }

    // Status with position
    std::string pos = "  Ln " + std::to_string(cy_) + ", Col " + std::to_string(cx_);
    screen_->draw_status(status_ + pos);

    // Place cursor at current position within viewport
    int screen_row = cy_ - row_off_;
    if (screen_row < 1) screen_row = 1;
    if (screen_row > max_rows) screen_row = max_rows;
    int screen_col = cx_;
    if (screen_col < 1) screen_col = 1;
    screen_->move_cursor(screen_row, screen_col);
    screen_->flush();
}

bool Editor::handle_input(int key) {
    if (key == input::KEY_CTRL_Q) {
        return false;
    }
    if (key == input::KEY_CTRL_C) {
        return false;
    }
    const auto& lines = buffer_->lines();
    auto clamp_col_to_line = [&](int y){
        if (y < 1) y = 1;
        if (y > (int)lines.size()) y = (int)lines.size();
        int len = (int)(y >= 1 && y <= (int)lines.size() ? lines[y-1].size() : 0);
        if (cx_ > len + 1) cx_ = len + 1;
    };

    switch (key) {
        case input::KEY_UP:
            if (cy_ > 1) cy_--;
            break;
        case input::KEY_DOWN:
            if (cy_ < (int)lines.size()) cy_++;
            break;
        case input::KEY_LEFT:
            if (cx_ > 1) {
                cx_--;
            } else if (cy_ > 1) {
                cy_--;
                cx_ = (int)lines[cy_-1].size() + 1;
            }
            break;
        case input::KEY_RIGHT: {
            int len = (int)(cy_ >= 1 && cy_ <= (int)lines.size() ? lines[cy_-1].size() : 0);
            if (cx_ <= len) {
                cx_++;
            } else if (cy_ < (int)lines.size()) {
                cy_++;
                cx_ = 1;
            }
            break;
        }
        default:
            break;
    }
    clamp_col_to_line(cy_);
    scroll();
    return true;
}

void Editor::scroll() {
    auto sz = screen_->size();
    int max_rows = sz.rows - 1; // minus status line
    if (cy_ < row_off_ + 1) {
        row_off_ = cy_ - 1;
        if (row_off_ < 0) row_off_ = 0;
    } else if (cy_ > row_off_ + max_rows) {
        row_off_ = cy_ - max_rows;
        if (row_off_ < 0) row_off_ = 0;
    }
}

} // namespace termite
