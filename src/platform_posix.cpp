#ifndef _WIN32

#include "termite/platform.hpp"

#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <csignal>
#include <cstdio>
#include <cstdlib>

namespace termite::platform
{

    namespace
    { // anonymous namespace -> methods and vars are only available in this file
        termios orig{};

        // terminal mode tracking. If  raw -> we control the terminal
        // if false, the terminal is controlled by the OS.
        bool raw = false;

        void restore()
        {
            if (raw)
            {
                tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig);
                raw = false;
            }
        }

        void on_sigint(int)
        {
            restore();
            ::_exit(130);
        }
    }

    bool init()
    {
        if (tcgetattr(STDIN_FILENO, &orig) == -1)
            return false; // read old terminal settings
        termios t = orig; // create copy of the terminal settings to modify
        // c_iflag -> input flags.
        t.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
        // c_oflag -> output flags.
        t.c_oflag &= ~(OPOST);
        t.c_cflag |= (CS8);
        // c_lflag -> visual flags.
        t.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
        // read behavior control
    // Allow detecting lone ESC by using a short timeout
    t.c_cc[VMIN] = 0;
    t.c_cc[VTIME] = 1; // 100ms
        if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &t) == -1)
            return false;

        // Enable alternate screen buffer to prevent scrolling
        std::printf("\033[?1049h");
        std::fflush(stdout);

        // setup signal to make sure terminal gets restored if user interupts
        std::signal(SIGINT, on_sigint);
        // restore terminal if application ends normally
        std::atexit(restore);
        raw = true;
        return true;
    }

    void shutdown()
    {
        restore();
    }

    int read_key()
    {
        unsigned char c;
        ssize_t n = ::read(STDIN_FILENO, &c, 1);
        if (n <= 0)
            return -1;
        return static_cast<int>(c);
    }

    std::optional<WinSize> window_size()
    {
        winsize ws{};
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1)
            return std::nullopt;
        return WinSize{.rows = static_cast<int>(ws.ws_row), .cols = static_cast<int>(ws.ws_col)};
    }

} // namespace termite::platform

#endif // _WIN32
