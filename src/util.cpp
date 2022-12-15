#include "util.h"
#include <range/v3/all.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

namespace r = ranges;
namespace rv = ranges::views;

/*------------------------------------------------------------------------------------------------*/

namespace {
    
}

std::vector<std::string> aoc::file_to_string_vector(const std::string& filename) {
    std::vector<std::string> v;

    std::ifstream fs(filename);
    if (!fs) {
        throw std::runtime_error("bad file");
    }

    std::string line;
    while (std::getline(fs, line)) {
        v.push_back(line);
    }
    return v;
}

std::string aoc::collapse_whitespace(const std::string& str){ 
    std::stringstream ss;
    for (auto i = str.begin(); i != str.end(); ++i) {
        ss << *i;
        if (std::isspace(*i)) {
            while (std::isspace(*(i+1))) {
                ++i;
            }
        }
    }
    return ss.str();
}

bool aoc::is_number(const std::string& s) {
    return !s.empty() && std::find_if(s.begin(),
        s.end(), [](unsigned char c) { return !std::isdigit(c); }) == s.end();
}

std::vector<std::string> aoc::split(const std::string& s, char delim) {
    std::stringstream ss(s);
    std::string item;
    std::vector<std::string> elems;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

std::string aoc::file_to_string(const std::string& filename) {
    std::ifstream file(filename);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::vector<std::vector<int>> aoc::strings_to_2D_array_of_digits(const std::vector<std::string>& lines) {
    std::vector<std::vector<int>> grid(lines.size());
    std::transform(lines.begin(), lines.end(), grid.begin(),
        [](const std::string& line)->std::vector<int> {
            std::vector<int> row(line.size());
            std::transform(line.begin(), line.end(), row.begin(),
                [](char c)->int {
                    return c - '0';
                }
            );
            return row;
        }
    );
    return grid;
}

std::vector<int> aoc::extract_numbers(const std::string& str, bool allow_negatives) {
    std::function<bool(char)> is_digit = (allow_negatives) ?
        [](char ch)->bool {return std::isdigit(ch); } :
        [](char ch)->bool {return std::isdigit(ch) || ch == '-'; };
    auto just_numbers = aoc::collapse_whitespace(
        str |
        rv::transform(
            [is_digit](char ch)->char {
                return (is_digit(ch)) ? ch : ' ';
            }
        ) | r::to<std::string>()
    );
    auto pieces = split(just_numbers, ' ');
    pieces = pieces |
        rv::remove_if([](auto&& str) {return str.empty(); }) |
        r::to_vector;
    return pieces |
        rv::transform(
            [](const auto& p)->int {
                return std::stoi(p);
            }
    ) | r::to_vector;
}

