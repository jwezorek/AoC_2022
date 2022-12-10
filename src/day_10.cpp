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

    struct statement {
        op_code op;
        int arg;
        statement(op_code op) : op(op), arg(0)
        {}

        statement(op_code op, int a) : op(op), arg(a)
        {}
    };

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

        bool contains_cycle(int cycle) const {
            return cycle >= start_cycle && cycle <= end_cycle;
        }
    };
    
    statement parse_line_of_input(const std::string& line) {
        auto pieces = aoc::split(line, ' ');
        static const std::unordered_map<std::string, op_code> str_to_op = {
            {"noop", op_code::noop},
            {"addx", op_code::addx}
        };
        auto op = (pieces[0] == "noop") ? op_code::noop : op_code::addx;
        if (pieces.size() == 2) {
            return { op, std::stoi(pieces[1]) };
        } else {
            return { op };
        }
    }

    int cycle_per_op(op_code op) {
        return (op == op_code::noop) ? 1 : 2;
    }

    int do_op(op_code op, int arg, int x_reg) {
        return (op == op_code::addx) ? arg + x_reg : x_reg;
    }

    std::tuple<cpu_state_range, cpu_state> execute_statement(
            const statement& statement, const cpu_state& state) {
        cpu_state_range cpu_states = {
            .x_register = state.x_register,
            .start_cycle = state.cycle + 1,
            .end_cycle = state.cycle + cycle_per_op(statement.op)
        };
        cpu_state next_cpu_state(
            do_op(statement.op, statement.arg, state.x_register), 
            cpu_states.end_cycle
        );
        return { cpu_states, next_cpu_state };
    }

    int sum_of_signal_strengths(const program& prog, const std::vector<int>& cycles) {
        cpu_state cpu;
        auto cycle_of_interest = cycles.begin();
        std::vector<int> signal_strengths;
        signal_strengths.reserve(cycles.size());

        for (const auto& statement : prog) {
            const auto& [range, next_state] = execute_statement(statement, cpu);
            if (cycle_of_interest != cycles.end() && range.contains_cycle(*cycle_of_interest)) {
                signal_strengths.push_back(*(cycle_of_interest++) * range.x_register);
            }
            cpu = next_state;
        }
        return r::accumulate(signal_strengths, 0);
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
                if (pos+1 == columns) {
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
    std::cout << "  part 1: " << 
        sum_of_signal_strengths(prog, { 20, 60, 100, 140, 180, 220 }) << "\n\n";
    std::cout << "  part 2: \n\n";
    std::cout << draw_rasters(prog);
}