#include "util.h"
#include "days.h"
#include <range/v3/all.hpp>
#include <iostream>
#include <filesystem>
#include <functional>
#include <tuple>
#include <sstream>

namespace r = ranges;
namespace rv = ranges::views;

/*------------------------------------------------------------------------------------------------*/

namespace{
    enum class op_code {
        noop,
        addx
    };
    using statement = std::tuple<op_code, int>;
    using program = std::vector<statement>;

    struct cpu_state {
        int x_register;
        int cycle;

        cpu_state(int x = 1, int c = 0) : x_register(x), cycle(c)
        {}
    };

    struct cpu_state_range {
        int x_register;
        int start_cycle;
        int end_cycle;
    };
    
    statement parse_line_of_input(const std::string& line) {
        auto pieces = aoc::split(line, ' ');
        auto op = (pieces[0] == "noop") ? op_code::noop : op_code::addx;
        int arg = (op == op_code::addx) ? std::stoi(pieces[1]) : 0;
        return { op, arg };
    }

    std::tuple<cpu_state_range, cpu_state> execute_statement(
            const statement& statement, const cpu_state& state) {
        auto [op, arg] = statement;
        cpu_state_range cpu_states = {
            .x_register = state.x_register,
            .start_cycle = state.cycle + 1,
            .end_cycle = state.cycle + ((op == op_code::noop) ? 1 : 2)
        };
        cpu_state next_cpu_state(
            (op == op_code::addx) ? arg + state.x_register : state.x_register,
            cpu_states.end_cycle
        );
        return { cpu_states, next_cpu_state };
    }

    int sum_of_signal_strengths(const program& prog) {
        cpu_state cpu;
        int sum_of_strengths = 0;
        for (const auto& statement : prog) {
            const auto& [range, next_state] = execute_statement(statement, cpu);
            for (int cycle = range.start_cycle; cycle <= range.end_cycle; ++cycle) {
                if (cycle % 40 == 20) {
                    sum_of_strengths += cycle * range.x_register;
                }
            }
            cpu = next_state;
        }
        return sum_of_strengths;
    }

    std::string draw_rasters(const program& prog) {
        constexpr auto columns = 40;
        std::stringstream ss;
        cpu_state cpu;

        ss << "  ";
        for (const auto& statement : prog) {
            const auto& [range, next_state] = execute_statement(statement, cpu);
            for (int cycle = range.start_cycle; cycle <= range.end_cycle; ++cycle) {
                int pos = (cycle - 1) % columns;
                char pixel = (pos >= range.x_register - 1 && pos <= range.x_register + 1) ? 
                    '#' : ' ';
                ss << pixel;
                if (pos == columns-1) {
                    ss << "\n  ";
                }
            }
            cpu = next_state;
        }
        ss << "\n";

        return ss.str();
    }
}

void aoc::day_10(int day, const std::string& title) {
    auto input = file_to_string_vector(input_path(day, 1));
    auto prog = input | rv::transform(parse_line_of_input) | r::to_vector;

    std::cout << header(day, title);
    std::cout << "  part 1: " << sum_of_signal_strengths(prog ) << "\n\n";
    std::cout << "  part 2: \n\n";
    std::cout << draw_rasters(prog);
}