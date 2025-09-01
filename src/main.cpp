#include <cstdio>
#include <cstdlib>

#include "../include/termite/editor.hpp"

int main(int argc, char** argv) {
    termite::Editor editor;
    return editor.run(argc, argv);
}

