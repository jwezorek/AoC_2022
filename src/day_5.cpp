#include "util.h"
#include "days.h"
#include <range/v3/all.hpp>
#include <vector>
#include <string>
#include <iostream>
#include <tuple>
#include <span>

namespace r = ranges;
namespace rv = ranges::views;

/*------------------------------------------------------------------------------------------------*/

namespace {

    using stacks = std::vector<std::vector<char>>;

    struct move {
        int amount;
        int from;
        int to;
    };

    std::vector<char> column_to_stack(const std::vector<std::string>& inp, int col) {
        int n = inp.size();
        return rv::iota(0, n) |
            rv::transform(
                [&inp, col](int row)->char {
                    return inp[row][4 * col + 1];
                }
            ) | rv::remove_if(
                [](char ch) {
                    return ch == ' ';
                }
            ) | r::to_vector;
    }

    stacks parse_stacks(std::span<const std::string> inp) {
        auto input = inp | rv::take(inp.size() - 1) | r::to_vector | r::actions::reverse;

        int n = (input.front().size() + 1) / 4;
        return rv::iota(0, n) |
            rv::transform(
                [&input](int i) {
                    return column_to_stack(input, i);
                }
            ) | r::to_vector;
    }

    std::vector<move> parse_moves(std::span<const std::string> inp) {
        auto input = inp | r::to_vector;
        return input | 
            rv::remove_if([](const auto& s) {return s.empty(); }) |
            rv::transform(
                [](const auto& line)->move {
                    auto strip_nonnumeric = line |
                        rv::remove_if(
                            [](auto ch) {
                                return !std::isdigit(ch) && ch != ' ';
                            }
                    ) | r::to<std::string>();
                    auto pieces = aoc::split(strip_nonnumeric, ' ');
                    pieces = pieces |
                        rv::remove_if([](const std::string& piece) {return piece.empty(); }) |
                        r::to_vector;
                    return {
                        std::stoi(pieces[0]),
                        std::stoi(pieces[1]) - 1,
                        std::stoi(pieces[2]) - 1
                    };
                }
            ) | r::to_vector;
    }

    std::tuple<stacks, std::vector<move>> parse_input(const std::vector<std::string>& input) {
        auto iter_sep = r::find_if(input, [](const auto& line) {return line.empty(); });
        return {
            parse_stacks({input.begin(), iter_sep}),
            parse_moves({iter_sep, input.end()})
        };
    }

    void move_one(stacks& s, int from, int to) {
        int item = s[from].back();
        s[from].pop_back();
        s[to].push_back(item);
    }

    void do_move_part1(stacks& s, const move& m) {
        for (int i = 0; i < m.amount; ++i) {
            move_one(s, m.from, m.to);
        }
    }

    std::string top_of_stacks(const stacks& s) {
        return s |
            rv::transform(
                [](const auto& stack)->char {
                    return stack.back();
                }
            ) | r::to<std::string>();
    }

    std::string do_part_one(const stacks& s, const std::vector<move>& moves) {
        stacks stacks = s;
        for (const auto& move : moves) {
            do_move_part1(stacks, move);
        }
        return top_of_stacks(stacks);
    }

    void do_move_part2(stacks& s, const move& m) {
        auto iter = s[m.from].end() - m.amount;
        std::copy(iter, s[m.from].end(), std::back_inserter(s[m.to]));
        s[m.from].erase(iter, s[m.from].end());
    }

    std::string do_part_two(const stacks& s, const std::vector<move>& moves) {
        stacks stacks = s;
        for (const auto& move : moves) {
            do_move_part2(stacks, move);
        }
        return top_of_stacks(stacks);
    }
}

void aoc::day_5(int day, const std::string& title) {
    auto input = file_to_string_vector(input_path(5, 1));
    auto [stacks, moves] = parse_input(input);
    std::cout << header(day, title);
    std::cout << "   part 1: " << do_part_one(stacks, moves) << "\n";
    std::cout << "   part 2: " << do_part_two(stacks, moves) << "\n";
}