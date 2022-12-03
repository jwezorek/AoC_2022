#include "util.h"
#include "days.h"
#include <range/v3/all.hpp>
#include <vector>
#include <string>
#include <iostream>
#include <unordered_map>
#include <array>
#include <tuple>

namespace r = ranges;
namespace rv = ranges::views;

/*------------------------------------------------------------------------------------------------*/

namespace {
    namespace rps {
        enum class shape : int {
            rock = 0, paper = 1, scissors = 2
        }; 

        enum class outcome : int {
            loss = 0, draw = 1, win = 2
        };
    }

    rps::shape letter_to_shape(char letter) {
        const static std::unordered_map<char, rps::shape> map_letter_to_shape = {
            {'A', rps::shape::rock}, {'B', rps::shape::paper}, {'C', rps::shape::scissors},
            {'X', rps::shape::rock}, {'Y', rps::shape::paper}, {'Z', rps::shape::scissors}
        };
        return map_letter_to_shape.at(letter);
    }

    rps::outcome player1_vs_player2_outcome(rps::shape p1, rps::shape p2) {
        const static std::array<std::array<rps::outcome, 3>, 3> win_loss_matrix = {{
            {rps::outcome::draw, rps::outcome::loss, rps::outcome::win},
            {rps::outcome::win, rps::outcome::draw, rps::outcome::loss},
            {rps::outcome::loss, rps::outcome::win, rps::outcome::draw}
        }};
        return win_loss_matrix[static_cast<int>(p1)][static_cast<int>(p2)];
    }

    int score_match(rps::shape p1, rps::shape p2) {
        auto outcome = player1_vs_player2_outcome(p1, p2);
        // (score for player 1's shape + 1) + 3 * (score for outcome);
        return (static_cast<int>(p1)+1) + 3*static_cast<int>(outcome);
    }

    std::tuple<rps::shape, rps::shape> parse_input_line_1(const std::string& inp_line) {
        auto p1_char = inp_line[2];
        auto p2_char = inp_line[0];
        return { letter_to_shape(p1_char), letter_to_shape(p2_char) };
    }

    // part 2...
    rps::outcome letter_to_outcome(char letter) {
        const static std::unordered_map<char, rps::outcome> map_letter_to_outcome = {
            {'X', rps::outcome::loss}, {'Y', rps::outcome::draw}, {'Z', rps::outcome::win}
        };
        return map_letter_to_outcome.at(letter);
    }

    std::tuple<rps::outcome, rps::shape> parse_input_line_2(const std::string& inp_line) {
        auto p2_char = inp_line[0];
        auto outcome_char = inp_line[2];
        return { letter_to_outcome(outcome_char), letter_to_shape(p2_char) };
    }

    rps::shape shape_to_yield_outcome(rps::shape player_2, rps::outcome desired_outcome) {
        const static std::array<std::array<rps::shape, 3>, 3> yield_outcome_matrix = {{
            {rps::shape::scissors, rps::shape::rock, rps::shape::paper},
            {rps::shape::rock, rps::shape::paper, rps::shape::scissors},
            {rps::shape::paper, rps::shape::scissors, rps::shape::rock }
        }};
        return yield_outcome_matrix[static_cast<int>(desired_outcome)][static_cast<int>(player_2)];
    }
}

void aoc::day_2(int day, const std::string& title) {
    auto input = file_to_string_vector(input_path(2, 1));
    auto total_score_part1 = r::accumulate(
        input |
            rv::transform(
                [](const auto& inp_line)->int {
                    auto [p1_shape, p2_shape] = parse_input_line_1(inp_line);
                    return score_match(p1_shape, p2_shape);
                }
            ),
        0
    );

    auto total_score_part2 = r::accumulate(
        input |
            rv::transform(
                [](const auto& inp_line)->int {
                    auto [desired_outcome, p2_shape] = parse_input_line_2(inp_line);
                    auto p1_shape = shape_to_yield_outcome(p2_shape, desired_outcome);
                    return score_match(p1_shape, p2_shape);
                }
            ),
        0
    );

    std::cout << header(day, title);
    std::cout << "  part 1: " << total_score_part1 << "\n";
    std::cout << "  part 2: " << total_score_part2 << "\n";
}