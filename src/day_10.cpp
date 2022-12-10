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
    using instruction = std::tuple<int, int>;
    using program = std::vector<instruction>;

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
    
    instruction parse_line_of_input(const std::string& line) {
        auto pieces = aoc::split(line, ' ');
        bool is_noop = pieces[0] == "noop";
        auto duration = is_noop ? 1 : 2;
        int incr = is_noop ? 0 : std::stoi(pieces[1]);
        return { duration, incr };
    }

    std::tuple<cpu_state_range, cpu_state> execute_statement(
            const instruction& instruction, const cpu_state& state) {
        auto [duration, increment] = instruction;
        cpu_state_range cpu_states = {
            .x_register = state.x_register,
            .start_cycle = state.cycle + 1,
            .end_cycle = state.cycle + duration
        };
        cpu_state next_cpu_state( state.x_register + increment, cpu_states.end_cycle);
        return { cpu_states, next_cpu_state };
    }

    int sum_of_signal_strengths(const program& prog) {
        cpu_state cpu;
        int sum_of_strengths = 0;
        for (const auto& instruction : prog) {
            const auto& [range, next_state] = execute_statement(instruction, cpu);
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
        for (const auto& instruction : prog) {
            const auto& [range, next_state] = execute_statement(instruction, cpu);
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