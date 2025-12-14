#pragma once

#include <memory>
#include <string>
#include <vector>

namespace termite {

class Screen;
class Buffer;

class Editor {
public:
    Editor();
    ~Editor();

    int run(int argc, char** argv);

private:
    void open_file_if_provided(int argc, char** argv);
    void render();
    bool handle_input(int key);
    void scroll();
    // Prompt for input on the status line; returns empty string if canceled.
    std::string prompt_input(const std::string& prompt, const std::string& initial = "");
    void clear_selection() { selecting_ = false; }
    void start_selection_if_needed() { if (!selecting_) { selecting_ = true; anchor_cx_ = cx_; anchor_cy_ = cy_; } }
    bool selection_active() const { return selecting_ && (anchor_cx_ != cx_ || anchor_cy_ != cy_); }
    void delete_selection();
    void copy_selection_to_clipboard();
    void paste_from_clipboard();
    void debug_note(const std::string& note);
    // Search helpers
    void start_search();
    void update_search_matches();
    void jump_to_match(int index);

    std::unique_ptr<Screen> screen_;
    std::unique_ptr<Buffer> buffer_;
    std::string status_;
    std::string filename_;
    bool modified_ {false};

    // Cursor position in buffer coordinates (1-based col, 1-based line index)
    int cx_ {1};
    int cy_ {1};
    // Vertical scroll offset: 0-based index of first visible line
    int row_off_ {0};
    //horizontal scoll offset
    int col_off_ {0};
    // Selection
    bool selecting_ {false};
    int anchor_cx_ {1};
    int anchor_cy_ {1};
    // Search state
    bool searching_ {false};
    std::string search_query_;
    struct Match { int line; int start; int end; }; // char indices
    std::vector<Match> search_matches_;
    int search_index_ { -1 };
    // Clipboard (internal)
    std::string clipboard_;
    // Debugging/status helpers
    std::string last_key_info_;
};

} 