#include "days.h"
#include "util.h"
#include <iostream>
#include <vector>
#include <functional>
#include <string>

/*------------------------------------------------------------------------------------------------*/

namespace {
    void do_advent_of_code_2022(int day) {
        const static auto advent_of_code_2022 = std::vector<std::function<void()>>{
            []() {aoc::day_1(1, "Calory Counting"); },
            []() {aoc::day_2(2, "Rock Paper Scissors"); },
            []() {aoc::day_3(3, "Rucksack Reorganization"); },
        };
        auto index = day - 1;
        if (index < advent_of_code_2022.size()) {
            advent_of_code_2022[index]();
        } else {
            std::cout << "Day " << day << " is not complete.\n";
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "no day specified\n";
        return -1;
    }
    std::string day = argv[1];
    if (!aoc::is_number(argv[1])) {
        std::cout << "invalid day: " << day << "\n";
        return -1;
    }
    auto day_number = std::stoi(day);
    if (day_number < 1 || day_number > 25) {
        std::cout << "invalid day: " << day << "\n";
        return -1;
    }

    do_advent_of_code_2022(day_number);
}

