#pragma once

#include <vector>
#include <string>
#include <stdexcept>

namespace aoc {
    std::vector<std::string> file_to_string_vector(const std::string& filename);
    bool is_number(const std::string& s);
}