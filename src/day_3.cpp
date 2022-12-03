#include "util.h"
#include "days.h"
#include <range/v3/all.hpp>
#include <vector>
#include <string>
#include <iostream>
#include <unordered_set>

namespace r = ranges;
namespace rv = ranges::views;

/*------------------------------------------------------------------------------------------------*/

namespace {
    std::tuple<std::string, std::string> split_string_in_half(const std::string& str) {
        auto n = str.size() / 2;
        return { str.substr(0, n), str.substr(n, n) };
    }

    char shared_item_letter(const std::string& input_line) {
        auto [rucksack_1,rucksack_2] = split_string_in_half(input_line);
        auto items_in_rucksack_1 = rucksack_1 | r::to<std::unordered_set<char>>();
        return *r::find_if(rucksack_2,
            [&items_in_rucksack_1](auto rucksack_2_item) {
                return items_in_rucksack_1.find(rucksack_2_item) != items_in_rucksack_1.end();
            }
        );
    }

    int priority_from_letter(char item) {
        return (std::islower(item)) ? (item - 'a') + 1 : (item - 'A') + 27;
    }

    // part 2 ...
    char shared_letter(auto rng) {
        auto letter_sets = rng |
            rv::transform(
                [](const std::string& inp_line) {
                    return inp_line | r::to<std::unordered_set<char>>();
                }
            ) | r::to_vector;
        for (char seed : rng[0]) {
            bool appears_in_all_sets = true;
            for (const auto& letter_set : letter_sets) {
                if (letter_set.find(seed) == letter_set.end()) {
                    appears_in_all_sets = false;
                    break;
                }
            }
            if (appears_in_all_sets) {
                return seed;
            }
        }
    }
}

void aoc::day_3(int day, const std::string& title) {
    auto input = file_to_string_vector(input_path(3, 1));
    auto part_1 = r::accumulate(
        input | rv::transform(
            [](const auto& inp_line)->int {
                return priority_from_letter(shared_item_letter(inp_line));
            }
        ),
        0
    );
    
    auto part_2 = r::accumulate(
        input |
            rv::chunk(3) |
            rv::transform(
                [](auto triple) {
                    return priority_from_letter(shared_letter(triple));
                }
            ),
        0
    );

    std::cout << header(day, title);
    std::cout << "  part 1: " << part_1 << "\n";
    std::cout << "  part 2: " << part_2 << "\n";
}