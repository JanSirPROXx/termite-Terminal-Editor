# Termite Text Editor
A lightweight, terminal-based text editor built in C++ with cross-platform support. Termite provides essential text editing features with a clean, responsive interface designed for efficient coding and text manipulation.

<img width="822" height="578" alt="image" src="https://github.com/user-attachments/assets/78076791-d31f-40be-a74e-22cd9530581e" />

## Features
- Multi-line text editing with cursor navigation
- File operations: Open, save, and save-as functionality
- Line numbers with automatic width adjustment
- Arrow key navigation (Up, Down, Left, Right)
- Word-wise navigation (Ctrl+Left/Right)
- Page navigation (Page Up/Down, Ctrl+Up/Down)
- Home/End keys for line beginning/end
- Text selection with Shift+Arrow keys
- Word selection with Ctrl+Shift+Arrow keys
- Interactive search (Ctrl+F)
- Search result navigation with Up/Down arrows during search
- Real-time match counting and highlighting
- Copy selection (Ctrl+Shift+C)
- Paste from clipboard (Ctrl+Shift+V)
- Horizontal scrolling for long lines

## Platform Support
The Platform specific parts are split into poisix and win, which alows The use on Windows and Linux Terminals.
The Editor is compatible with xterm and rxvt Terminals (rxvt has less features yet)

## Dependencies
C++ 17
CMAKE v3.28
Standard library only (no external dependencies)
