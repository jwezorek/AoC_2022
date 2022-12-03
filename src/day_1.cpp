#include "util.h"
#include "days.h"
#include <range/v3/all.hpp>
#include <iostream>
#include <filesystem>
#include <functional>

namespace r = ranges;
namespace rv = ranges::views;

/*------------------------------------------------------------------------------------------------*/

void aoc::day_1(int day, const std::string& title) {
    auto input = file_to_string_vector( input_path(day,1) );
    auto sorted_calories =
        input | 
        rv::chunk_by([](auto lhs, auto rhs) {return is_number(lhs) && is_number(rhs); }) |
        rv::remove_if([](auto group) {return group.front().empty(); }) |
        rv::transform(
            [](auto group) {
                return r::accumulate(
                    group | rv::transform([](const auto& str) {return std::stoi(str); }), 0
                );
            }
        ) |
        r::to_vector |
        r::actions::sort(std::greater<int>());

    auto max_calories_1_elf = sorted_calories.front();
    auto max_calories_3_elves = r::accumulate( sorted_calories | rv::take(3), 0);

    std::cout << header(day, title);
    std::cout << "  part 1: " << max_calories_1_elf << "\n";
    std::cout << "  part 2: " << max_calories_3_elves << "\n";
}