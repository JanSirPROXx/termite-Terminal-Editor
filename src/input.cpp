#include "termite/input.hpp"
#include "termite/platform.hpp"

namespace termite::input {

static int read_byte() { return platform::read_key(); }

int read_key() {
    int c = read_byte();
    if (c == '\r' || c == '\n') return KEY_ENTER;
    if (c == 127 || c == 8) return KEY_BACKSPACE;
    if (c != 27) { // Not an escape character, treat as normal byte
        return c; //Also all CTR_a, .... normal ctrl characters
    }
    // Attempt to parse common escape sequences.
    int c1 = read_byte();
    if (c1 == -1) return 27; //probably only an escape character :)
    if (c1 == '[') { //xterm Terminal
        int c2 = read_byte();
        if (c2 == -1) return 27;
        // Simple arrows H/F and A-D
        if (c2 >= 'A' && c2 <= 'Z') {
            switch (c2) {
                case 'A': return KEY_UP;
                case 'B': return KEY_DOWN;
                case 'C': return KEY_RIGHT;
                case 'D': return KEY_LEFT;
                case 'H': return KEY_HOME;
                case 'F': return KEY_END;
                default: break;
            }
        }
        // Numeric or CSI parameterized sequences
        if (c2 >= '0' && c2 <= '9') {
            int p1 = c2 - '0';
            int ch;
            // accumulate first parameter
            while (true) {
                ch = read_byte();
                if (ch < 0) return KEY_UNKNOWN;
                if (ch >= '0' && ch <= '9') { p1 = p1 * 10 + (ch - '0'); continue; }
                break;
            }
            if (ch == '~') {
                switch (p1) {
                    case 1: return KEY_HOME;  // xterm
                    case 4: return KEY_END;
                    case 5: return KEY_PAGE_UP;
                    case 6: return KEY_PAGE_DOWN;
                    case 2: return KEY_INSERT;
                    case 3: return KEY_DELETE;
                    case 7: return KEY_HOME;  // rxvt
                    case 8: return KEY_END;   // rxvt
                    case 12: return KEY_F2;   // common F2 mapping
                    default: return KEY_UNKNOWN;
                }
            } else if (ch == ';') {
                // Modifier parameter, e.g., ESC [ 1 ; 5 D (Ctrl-Left)
                int p2 = 0;
                ch = read_byte();
                if (ch < 0) return KEY_UNKNOWN;
                if (ch >= '0' && ch <= '9') {
                    p2 = ch - '0';
                    // read any additional digits of p2
                    while (true) {
                        ch = read_byte();
                        if (ch < 0) return KEY_UNKNOWN;
                        if (ch >= '0' && ch <= '9') { p2 = p2 * 10 + (ch - '0'); continue; }
                        break;
                    }
                    // ch should now be the final letter
                    if (p2 == 5) { // Ctrl modifier
                        switch (ch) {
                            case 'A': return KEY_CTRL_UP;
                            case 'B': return KEY_CTRL_DOWN;
                            case 'C': return KEY_CTRL_RIGHT;
                            case 'D': return KEY_CTRL_LEFT;
                            case 'H': return KEY_CTRL_HOME;
                            case 'F': return KEY_CTRL_END;
                            default: break;
                        }
                    } else if (p2 == 2) { // Shift modifier
                        switch (ch) {
                            case 'A': return KEY_SHIFT_UP;
                            case 'B': return KEY_SHIFT_DOWN;
                            case 'C': return KEY_SHIFT_RIGHT;
                            case 'D': return KEY_SHIFT_LEFT;
                            default: break;
                        }
                    } else if (p2 == 6) { // Ctrl+Shift
                        switch (ch) {
                            case 'A': return KEY_CTRL_SHIFT_UP;
                            case 'B': return KEY_CTRL_SHIFT_DOWN;
                            case 'C': return KEY_CTRL_SHIFT_RIGHT;
                            case 'D': return KEY_CTRL_SHIFT_LEFT;
                            case 'H': return KEY_CTRL_HOME; // treat as extend-to-home
                            case 'F': return KEY_CTRL_END;  // treat as extend-to-end
                            default: break;
                        }
                    }
                }
            }
        }
        return KEY_UNKNOWN;
    } else if (c1 == 'O') { //rxvt Terminal 
        int c2 = read_byte();
        if (c2 == -1) return 27;
        switch (c2) {
            case 'A': return KEY_UP;
            case 'B': return KEY_DOWN;
            case 'C': return KEY_RIGHT;
            case 'D': return KEY_LEFT;
            case 'H': return KEY_HOME;
            case 'F': return KEY_END;
            default:
                return KEY_UNKNOWN;
        }
    }
    return KEY_UNKNOWN;
}

} // namespace termite::input
