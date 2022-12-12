#pragma once

#include <string>
#include <optional>
/*------------------------------------------------------------------------------------------------*/

namespace aoc {
    std::string input_path(int day, std::optional<int> part);
    std::string header(int day, const std::string& title);

    void day_1(const std::string& title);
    void day_2(const std::string& title);
    void day_3(const std::string& title);
    void day_4(const std::string& title);
    void day_5(const std::string& title);
    void day_6(const std::string& title);
    void day_7(const std::string& title);
    void day_8(const std::string& title);
    void day_9(const std::string& title);
    void day_10(const std::string& title);
    void day_11(const std::string& title);
    void day_12(const std::string& title);
}