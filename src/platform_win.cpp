#ifdef _WIN32
//platform specific code for windows terminal

#include "termite/platform.hpp"

#include <windows.h>

namespace termite::platform {

static DWORD orig_mode = 0;
static bool initialized = false;

bool init() {
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    if (!GetConsoleMode(hIn, &orig_mode)) return false;
    DWORD mode = orig_mode;
    mode &= ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT);
    if (!SetConsoleMode(hIn, mode)) return false;
    initialized = true;
    return true;
}

void shutdown() {
    if (initialized) {
        HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
        SetConsoleMode(hIn, orig_mode);
        initialized = false;
    }
}

int read_key() {
    INPUT_RECORD rec{};
    DWORD read = 0;
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    while (ReadConsoleInput(hIn, &rec, 1, &read)) {
        if (rec.EventType == KEY_EVENT && rec.Event.KeyEvent.bKeyDown) {
            return static_cast<int>(rec.Event.KeyEvent.uChar.AsciiChar);
        }
    }
    return -1;
}

std::optional<WinSize> window_size() {
    CONSOLE_SCREEN_BUFFER_INFO info{};
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (!GetConsoleScreenBufferInfo(hOut, &info)) return std::nullopt;
    int rows = info.srWindow.Bottom - info.srWindow.Top + 1;
    int cols = info.srWindow.Right - info.srWindow.Left + 1;
    return WinSize{.rows = rows, .cols = cols};
}

} // namespace termite::platform

#endif // _WIN32

