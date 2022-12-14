#include "util.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

/*------------------------------------------------------------------------------------------------*/

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