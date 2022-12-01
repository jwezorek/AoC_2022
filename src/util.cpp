#include "util.h"
#include <iostream>
#include <fstream>

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

bool aoc::is_number(const std::string& s) {
    return !s.empty() && std::find_if(s.begin(),
        s.end(), [](unsigned char c) { return !std::isdigit(c); }) == s.end();
}
