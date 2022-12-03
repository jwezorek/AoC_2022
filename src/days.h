#pragma once

#include <string>
/*------------------------------------------------------------------------------------------------*/

namespace aoc {
    std::string input_path(int day, int part);
    std::string header(int day, const std::string& title);

    void day_1(int day, const std::string& title);
    void day_2(int day, const std::string& title);
    void day_3(int day, const std::string& title);
}