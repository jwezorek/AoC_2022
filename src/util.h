#pragma once

#include <vector>
#include <string>
#include <stdexcept>

namespace aoc {
    std::vector<std::string> file_to_string_vector(const std::string& filename);
    std::string file_to_string(const std::string& filename);
    std::string collapse_whitespace(const std::string& str);
    bool is_number(const std::string& s);
    std::vector<std::string> split(const std::string& s, char delim);
    std::vector<std::vector<int>> strings_to_2D_array_of_digits(const std::vector<std::string>& lines);
    std::vector<int> extract_numbers(const std::string& str);
}