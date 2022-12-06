#include "util.h"
#include "days.h"
#include <range/v3/all.hpp>
#include <vector>
#include <string>
#include <iostream>
#include <tuple>
#include <unordered_set>

namespace r = ranges;
namespace rv = ranges::views;

/*------------------------------------------------------------------------------------------------*/

namespace {

    bool are_unique_letters(auto letters) {
        std::unordered_set<char> letters_seen;
        for (char letter : letters) {
            if (letters_seen.find(letter) != letters_seen.end()) {
                return false;
            }
            letters_seen.insert(letter);
        }
        return true;
    }

    int first_appearance_of_n_unique_letters(const std::string& input, int n) {
        auto pair = r::find_if(
            rv::enumerate(input) | rv::sliding(n),
            []( auto rng_of_tups)->bool {
                return are_unique_letters(
                    rng_of_tups | rv::transform( [](auto tup) {  return std::get<1>(tup); })
                );
             }
        );
        auto [index, last_letter] = (*pair)[n-1];
        return index + 1;
    }
}

void aoc::day_6(int day, const std::string& title) {
    auto input = file_to_string(input_path(6, 1));
    std::cout << header(day, title);
    std::cout << "   part 1: " << 
        first_appearance_of_n_unique_letters(input, 4) << "\n";
    std::cout << "   part 2: " <<
        first_appearance_of_n_unique_letters(input, 14) << "\n";
}