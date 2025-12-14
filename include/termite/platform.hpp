#pragma once

#include <optional>

namespace termite::platform {

struct WinSize { int rows{24}; int cols{80}; };

// Initialize terminal I/O (e.g., enter raw mode on POSIX)
bool init();

// Restore terminal I/O
void shutdown();

// Read a single key (blocking). Returns byte value or -1 on error.
int read_key();

// Query current terminal window size.
std::optional<WinSize> window_size();

} // namespace termite::platform

