#pragma once

#include <string>
#include <optional>
/*------------------------------------------------------------------------------------------------*/

namespace aoc {
    std::string input_path(int day, std::optional<int> part);
    std::string header(int day, const std::string& title);

    void day_1(int day, const std::string& title);
    void day_2(int day, const std::string& title);
    void day_3(int day, const std::string& title);
    void day_4(int day, const std::string& title);
    void day_5(int day, const std::string& title);
    void day_6(int day, const std::string& title);
    void day_7(int day, const std::string& title);
    void day_8(int day, const std::string& title);
    void day_9(int day, const std::string& title);
    void day_10(int day, const std::string& title);
}