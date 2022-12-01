#include "util.h"
#include "days.h"
#include <range/v3/all.hpp>
#include <iostream>
#include <filesystem>

namespace r = ranges;
namespace rv = ranges::views;

/*------------------------------------------------------------------------------------------------*/

void aoc::day_1() {
    auto input = file_to_string_vector( input_path(1,1) );
    auto calories_per_elf =
        input | 
        rv::chunk_by([](auto lhs, auto rhs) {return is_number(lhs) && is_number(rhs); }) |
        rv::remove_if([](auto group) {return group.front().empty(); }) |
        rv::transform(
            [](auto group) {
                return r::accumulate(
                    group | rv::transform([](const auto& str) {return std::stoi(str); }), 0
                );
            }
        ) | r::to_vector;

    auto max_calories_1_elf = r::max(calories_per_elf);
    auto max_calories_3_elves = r::accumulate(
        rv::reverse(rv::all(calories_per_elf) | r::actions::sort) | rv::take(3), 0
    );

    std::cout << header(1);
    std::cout << "  part 1: " << max_calories_1_elf << "\n";
    std::cout << "  part 2: " << max_calories_3_elves << "\n";
}