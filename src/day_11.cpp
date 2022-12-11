#include "util.h"
#include "days.h"
#include <range/v3/all.hpp>
#include <iostream>
#include <filesystem>
#include <functional>
#include <regex>
#include <sstream>
#include <deque>

namespace r = ranges;
namespace rv = ranges::views;

/*------------------------------------------------------------------------------------------------*/

namespace {

    using operation_fn = std::function<int64_t(int64_t)>;

    struct monkey {
        int index;
        std::deque<int64_t> items;
        operation_fn operation;
        int test_divisor;
        int if_true_monkey;
        int if_false_monkey;
        int64_t inspection_count;
    };

    std::vector<std::string> trim_extraneous_text(const std::string& str) {
        if (str.find("Operation") != std::string::npos) {
            return aoc::split(str.substr(str.find("=") + 2), ' ');
        }
        auto words = aoc::split(str, ' ');
        return words |
            rv::transform(
                [](const std::string& str)->std::string {
                    return std::regex_replace(str, std::regex(R"([\D])"), "");
                }
            ) |
            rv::remove_if(
                [](const std::string& str) {
                    return str.empty();
                }
            ) | r::to_vector;

    }

    operation_fn parse_operation(const std::vector<std::string>& pieces) {
        auto parse_arg = [](const std::string& str)->operation_fn {
            return (str == "old") ?
                operation_fn{ [](int64_t old)->int64_t {return old; } } :
                operation_fn{ [arg = std::stoi(str)] (int64_t)->int64_t{return arg; } };
        };
        auto lhs = parse_arg(pieces[0]);
        auto rhs = parse_arg(pieces[2]);
        return (pieces[1] == "*") ?
            operation_fn{ [lhs,rhs](int64_t old)->int64_t {return lhs(old) * rhs(old); } } :
            operation_fn{ [lhs,rhs](int64_t old)->int64_t {return lhs(old) + rhs(old); } };
    }

    std::vector<monkey> parse_input(const std::vector<std::string>& input) {
        return input |
            rv::chunk_by([](auto lhs, auto rhs) {return (!lhs.empty()) && (!rhs.empty()); }) |
            rv::remove_if([](auto group) {return group.front().empty(); }) |
            rv::transform(
                [](auto group)->monkey {
                    auto lines = group | rv::transform(trim_extraneous_text) | r::to_vector;
                    return {
                        .index = std::stoi(lines[0].front()),
                        .items = lines[1] | 
                            rv::transform([](auto&& str)->int64_t {return std::stoi(str); }) | 
                            r::to<std::deque<int64_t>>(),
                        .operation = parse_operation(lines[2]),
                        .test_divisor = std::stoi(lines[3].front()),
                        .if_true_monkey = std::stoi(lines[4].front()),
                        .if_false_monkey = std::stoi(lines[5].front()),
                        .inspection_count = 0
                    };
                }
            ) |
            r::to_vector;
    }

    void perform_turn(std::vector<monkey>& monkeys, int n, std::optional<int64_t> modulus) {
        auto& monkey = monkeys[n];
        while (!monkey.items.empty()) {
            monkey.inspection_count++;
            auto item = monkey.items.front();
            monkey.items.pop_front();

            item = monkey.operation(item);
            if (modulus) {
                item = item % modulus.value();
            } else {
                item /= 3;
            }

            int dest_monkey = (item % monkey.test_divisor == 0) ? 
                monkey.if_true_monkey : monkey.if_false_monkey;
            monkeys[dest_monkey].items.push_back(item);
        }
    }

    void perform_round(std::vector<monkey>& monkeys, std::optional<int64_t> modulus) {
        int n = static_cast<int>(monkeys.size());
        for (int i = 0; i < n; ++i) {
            perform_turn(monkeys, i, modulus);
        }
    }

    int64_t calculate_modulus(const std::vector<monkey>& monkeys) {
        // the test divisors are all prime so LCM is just their product...
        return r::accumulate(
            monkeys | rv::transform([](auto&& m)->int64_t {return m.test_divisor; }),
            static_cast<int64_t>(1),
            std::multiplies<int64_t>()
        );
    }

    int64_t level_of_monkey_business(const std::vector<monkey>& input_monkeys, 
            int num_rounds, std::optional<int64_t> modulus) {
        auto monkeys = input_monkeys;
        for (int i = 0; i < num_rounds; i++) {
            perform_round(monkeys, modulus);
        }
        auto sorted_counts = monkeys | 
            rv::transform([](auto&& m) {return m.inspection_count; }) |
            r::to_vector |
            r::actions::sort(std::greater<int64_t>());

        return sorted_counts[0] * sorted_counts[1];
    }

}

void aoc::day_11(const std::string& title) {
    auto input = file_to_string_vector(input_path(11, 1));
    auto monkeys = parse_input(input);
    std::cout << header(11, title);
    std::cout << "  part 1: " << level_of_monkey_business(monkeys, 20, {}) << "\n";
    auto modulo = calculate_modulus(monkeys);
    std::cout << "  part 2: " << level_of_monkey_business(monkeys, 10000, modulo) << "\n";
}