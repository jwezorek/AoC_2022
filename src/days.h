#pragma once

#include <string>
/*------------------------------------------------------------------------------------------------*/

namespace aoc {
    std::string input_path(int day, int part);
    std::string header(int day, const std::string& title);

    void day_1();
    void day_2();
}