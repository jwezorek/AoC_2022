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
            []() {aoc::day_1(  "Calory Counting" ); },
            []() {aoc::day_2(  "Rock Paper Scissors" ); },
            []() {aoc::day_3(  "Rucksack Reorganization" ); },
            []() {aoc::day_4(  "Camp Cleanup" ); },
            []() {aoc::day_5(  "Supply Stacks" ); },
            []() {aoc::day_6(  "Tuning Trouble" ); },
            []() {aoc::day_7(  "No Space Left On Device" ); },
            []() {aoc::day_8(  "Treetop Tree House" ); },
            []() {aoc::day_9(  "Rope Bridge" ); },
            []() {aoc::day_10( "Cathode-Ray Tube" ); },
            []() {aoc::day_11( "Monkey in the Middle" ); },
            []() {aoc::day_12( "Hill Climbing Algorithm" ); },
            []() {aoc::day_13( "TODO" ); },
            []() {aoc::day_14( "TODO" ); },
            []() {aoc::day_15( "TODO" ); },
            []() {aoc::day_16( "TODO" ); },
            []() {aoc::day_17( "TODO" ); },
            []() {aoc::day_18( "TODO" ); },
            []() {aoc::day_19( "TODO" ); },
            []() {aoc::day_20( "TODO" ); },
            []() {aoc::day_21( "TODO" ); },
            []() {aoc::day_22( "TODO" ); },
            []() {aoc::day_23( "TODO" ); },
            []() {aoc::day_24( "TODO" ); },
            []() {aoc::day_25( "TODO" ); }
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

