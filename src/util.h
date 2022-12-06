#pragma once

#include <vector>
#include <string>
#include <stdexcept>

namespace aoc {
    std::vector<std::string> file_to_string_vector(const std::string& filename);
    std::string file_to_string(const std::string& filename);
    bool is_number(const std::string& s);
    std::vector<std::string> split(const std::string& s, char delim);
}