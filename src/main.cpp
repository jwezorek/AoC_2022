#include "days.h"
#include "util.h"
#include <iostream>
#include <vector>
#include <functional>
#include <string>

/*------------------------------------------------------------------------------------------------*/

namespace {
    int do_advent_of_code_2022(int day) {
        const static auto advent_of_code_2022 = std::vector<std::function<void()>>{
            [](){ aoc::day_1(1, "Calory Counting"); },
            [](){ aoc::day_2(2, "Rock Paper Scissors"); },
            [](){ aoc::day_3(3, "Rucksack Reorganization"); },
            [](){ aoc::day_4(4, "Camp Cleanup"); },
            [](){ aoc::day_5(5, "Supply Stacks"); },
            [](){ aoc::day_6(6, "Tuning Trouble"); },
            [](){ aoc::day_7(7, "No Space Left On Device"); },
            [](){ aoc::day_8(8, "Treetop Tree House"); },
            [](){ aoc::day_9(9, "Rope Bridge"); },
            [](){ aoc::day_10(10, "Cathode-Ray Tube"); }
        };
        auto index = day - 1;
        if (index < 0) {
            std::cout << "invalid day: " << day << "\n";
            return -1;
        }

        if (index < advent_of_code_2022.size()) {
            advent_of_code_2022[index]();
        } else {
            std::cout << "Day " << day << " is not complete.\n";
        }

        return 0;
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
        std::cout << "invalid day: " << day_number << "\n";
        return -1;
    }

    return do_advent_of_code_2022(day_number);
}

