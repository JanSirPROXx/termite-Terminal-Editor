#pragma once
#include <string>

namespace termite { class Buffer; }

namespace termite::file_io {

std::string read_file(const std::string& path);
bool save_file(const Buffer& buffer, const std::string& path);

}
