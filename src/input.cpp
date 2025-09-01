#include "termite/input.hpp"
#include "termite/platform.hpp"

namespace termite::input {

static int read_byte() { return platform::read_key(); }

int read_key() {
    int c = read_byte();
    if (c != 27) {
        return c;
    }
    // Attempt to parse escape sequences for arrows: ESC [ A/B/C/D or ESC O A/B/C/D
    int c1 = read_byte();
    if (c1 == -1) return 27;
    if (c1 == '[') {
        int c2 = read_byte();
        if (c2 == -1) return 27;
        switch (c2) {
            case 'A': return KEY_UP;
            case 'B': return KEY_DOWN;
            case 'C': return KEY_RIGHT;
            case 'D': return KEY_LEFT;
            default:
                return KEY_UNKNOWN;
        }
    } else if (c1 == 'O') {
        int c2 = read_byte();
        if (c2 == -1) return 27;
        switch (c2) {
            case 'A': return KEY_UP;
            case 'B': return KEY_DOWN;
            case 'C': return KEY_RIGHT;
            case 'D': return KEY_LEFT;
            default:
                return KEY_UNKNOWN;
        }
    }
    return KEY_UNKNOWN;
}

} // namespace termite::input
