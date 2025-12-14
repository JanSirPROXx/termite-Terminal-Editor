#pragma once

namespace termite::input {

// Key codes (very small subset yet)
enum : int {
    KEY_CTRL_A = 1,
    KEY_CTRL_B = 2,
    KEY_CTRL_D = 4,
    KEY_CTRL_E = 5,
    KEY_CTRL_G = 7,
    KEY_UNKNOWN = -1,
    KEY_CTRL_C = 3,
    KEY_CTRL_Q = 17,
    KEY_UP = 1001,
    KEY_DOWN = 1002,
    KEY_LEFT = 1003,
    KEY_RIGHT = 1004,
    KEY_HOME = 1005,
    KEY_END = 1006,
    KEY_PAGE_UP = 1007,
    KEY_PAGE_DOWN = 1008,
    KEY_INSERT = 1009,
    KEY_DELETE = 1010,
    KEY_BACKSPACE = 1011,
    KEY_ENTER = 1012,
    KEY_CTRL_S = 19,
    KEY_CTRL_LEFT = 1101,
    KEY_CTRL_RIGHT = 1102,
    KEY_CTRL_UP = 1103,
    KEY_CTRL_DOWN = 1104,
    KEY_CTRL_HOME = 1105,
    KEY_CTRL_END = 1106,
    KEY_CTRL_V = 22,
    KEY_CTRL_F = 6,
    KEY_SHIFT_LEFT = 1201,
    KEY_SHIFT_RIGHT = 1202,
    KEY_SHIFT_UP = 1203,
    KEY_SHIFT_DOWN = 1204,
    KEY_CTRL_SHIFT_LEFT = 1205,
    KEY_CTRL_SHIFT_RIGHT = 1206,
    KEY_CTRL_SHIFT_UP = 1207,
    KEY_CTRL_SHIFT_DOWN = 1208,
    KEY_CTRL_K = 11,
    KEY_F2 = 2002,
};

int read_key();

}
