cmake_minimum_required(VERSION 3.16)

project(termite VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(TERMIT_ENABLE_WARNINGS "Enable strict warnings" ON)

set(TERMITE_SOURCES
    src/main.cpp
    src/editor.cpp
    src/editor_render.cpp
    src/editor_input.cpp
    src/buffer.cpp
    src/gap_buffer.cpp
    src/screen.cpp
    src/input.cpp
    src/file_io.cpp
)

# Platform-specific source
if(WIN32)
    list(APPEND TERMITE_SOURCES src/platform_win.cpp)
else()
    list(APPEND TERMITE_SOURCES src/platform_posix.cpp)
endif()

add_executable(termite ${TERMITE_SOURCES})

target_include_directories(termite PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Expose version to the code
target_compile_definitions(termite PRIVATE TERMITE_VERSION="0.1")

if(TERMIT_ENABLE_WARNINGS)
    if(MSVC)
        target_compile_options(termite PRIVATE /W4 /permissive-)
    else()
        target_compile_options(termite PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()

if(UNIX AND NOT APPLE)
    # Link against termcap-like libraries if needed; commonly not necessary for raw termios
    # target_link_libraries(termite PRIVATE tinfo ncurses)
endif()
