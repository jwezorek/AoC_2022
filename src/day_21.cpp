#include "util.h"
#include "days.h"
#include <range/v3/all.hpp>
#include <iostream>
#include <functional>
#include <variant>
#include <sstream>
#include <unordered_map>
#include <stack>

namespace r = ranges;
namespace rv = ranges::views;

/*------------------------------------------------------------------------------------------------*/

namespace {

    template <class... Ts>
    struct overloaded : Ts... {
        using Ts::operator()...;
    };

    template <class... Ts>
    overloaded(Ts...)->overloaded<Ts...>;

    struct binary_expression {
        char op;
        std::string lhs;
        std::string rhs;
    };
    using expression = std::variant<int64_t, binary_expression>;

    struct variable_def {
        std::string var;
        expression expr;
    };

    variable_def str_to_variable_def(const std::string& line) {
        auto [var, def] = aoc::split_to_tuple<2>(line, ':');
        def = aoc::trim(def);
        if (aoc::is_number(def)) {
            return { var, std::stoi(def) };
        }
        auto [lhs, op, rhs] = aoc::split_to_tuple<3>(def, ' ');
        return { var, {binary_expression{op[0], lhs, rhs}} };
    }

    std::string to_string(const binary_expression& expr) {
        std::stringstream ss;
        ss << expr.lhs << " " << expr.op << " " << expr.rhs;
        return ss.str();
    }

    std::string to_string(const variable_def& def) {
        std::string expr_str = std::visit(
            overloaded{
                [](int64_t num)->std::string { return std::to_string(num); },
                [](const binary_expression& expr) { return to_string(expr); }
            },
            def.expr
        );
        return def.var + std::string(" : ") + expr_str;
    }

    using eval_stack_item = std::variant<int64_t, char, std::string>;
    using binary_op = std::function < int64_t(int64_t, int64_t)>;

    using var_def_tbl = std::unordered_map<std::string, expression>;
    std::optional<int64_t> evaluate_variable(const var_def_tbl& definitions, const std::string& var) {
        const static std::unordered_map<char, binary_op> op_tbl = {
            {'+', [](int64_t lhs, int64_t rhs)->int64_t { return lhs + rhs; }},
            {'-', [](int64_t lhs, int64_t rhs)->int64_t { return lhs - rhs; }},
            {'*', [](int64_t lhs, int64_t rhs)->int64_t { return lhs * rhs; }},
            {'/', [](int64_t lhs, int64_t rhs)->int64_t { return lhs / rhs; }}
        };

        std::stack<int64_t> arg_stack;
        std::stack<eval_stack_item> eval_stack;
        eval_stack.push({ var });
        while (eval_stack.size() > 1 || !std::holds_alternative<int64_t>(eval_stack.top())) {
            auto item = eval_stack.top();
            eval_stack.pop();
            if (std::holds_alternative<std::string>(item)) {
                if (std::get<std::string>(item) == "<unknown-var>") {
                    return {};
                }
            }
            std::visit(
                overloaded{
                    [&](int64_t num) { 
                        arg_stack.push(num); 
                    },
                    [&](char op) {
                        auto func = op_tbl.at(op);
                        auto arg1 = arg_stack.top();
                        arg_stack.pop();
                        auto arg2 = arg_stack.top();
                        arg_stack.pop();
                        eval_stack.push(func(arg1,arg2));
                    },
                    [&](const std::string& var) {
                        if (!definitions.contains(var)) {
                            return eval_stack.push("<unknown-var>");
                        }
                        auto val = definitions.at(var);
                        if (std::holds_alternative<int64_t>(val)) {
                            eval_stack.push(std::get<int64_t>(val));
                        } else {
                            auto expr = std::get<binary_expression>(val);
                            eval_stack.push(expr.op);
                            eval_stack.push(expr.lhs);
                            eval_stack.push(expr.rhs);
                        }
                    }
                },
                item
            );
        }
        
        return std::get<int64_t>(eval_stack.top());
    }
    struct binary_expr_def {
        std::string var;
        binary_expression expr;
    };

    std::optional<binary_expr_def> find_unknown_expression(
            const var_def_tbl& definitions, const std::string& variable) {
        std::optional<binary_expr_def> target = {};
        for (const auto& var_def : definitions ) {
            const auto [var, expr] = var_def;
            if (std::holds_alternative<binary_expression>(expr)) {
                const auto& bin_expr = std::get<binary_expression>(expr);
                if (bin_expr.lhs == variable || bin_expr.rhs == variable) {
                    if (target.has_value()) {
                        throw std::runtime_error(
                            "this algorithm in not sophisticated enought to solve this input"
                        );
                    }
                    target = { var, bin_expr };
                }
            }
        }
        return target;
    }

    variable_def root_expression_part2(const var_def_tbl& definitions) {
        auto root_expr = std::get<binary_expression>(definitions.at("root"));
        auto defs = definitions;
        defs.erase("humn");
        auto lhs = evaluate_variable(defs, root_expr.lhs);
        auto rhs = evaluate_variable(defs, root_expr.rhs);

        if (lhs) {
            return { root_expr.rhs, *lhs };
        } else {
            return { root_expr.lhs, *rhs };
        }
    }

    std::vector<variable_def> solve_for_x(const binary_expr_def& def, std::string unknown, const var_def_tbl& defs) {
        const auto& expr = def.expr;
        if (expr.op == '-') {
            if (expr.lhs == unknown) {
                auto value = evaluate_variable(defs, expr.rhs);
                return {
                    variable_def{expr.rhs, *value},
                    variable_def{unknown, binary_expression{ '+', def.var, expr.rhs} }
                };
            } else {
                auto value = evaluate_variable(defs, expr.lhs);
                return {
                    variable_def{expr.lhs, *value},
                    variable_def{unknown, binary_expression{'-', expr.lhs, def.var}}
                };
            }
        } else if (expr.op == '/') {
            if (expr.lhs == unknown) {
                auto value = evaluate_variable(defs, expr.rhs);
                return {
                    variable_def{expr.rhs, *value},
                    variable_def{unknown, binary_expression{ '*', def.var, expr.rhs} }
                };
            } else {
                auto value = evaluate_variable(defs, expr.lhs);
                return {
                    variable_def{expr.lhs, *value},
                    variable_def{unknown, binary_expression{'/', expr.lhs, def.var}}
                };
            }
        } else if (expr.op == '+') {
            auto num = (expr.lhs == unknown) ?
                *evaluate_variable(defs, expr.rhs) :
                *evaluate_variable(defs, expr.lhs);
            auto set_var = (expr.lhs == unknown) ? expr.rhs : expr.lhs;
            return {
                variable_def{set_var, num},
                variable_def{unknown, binary_expression{'-', def.var, set_var}}
            };
        } else if (expr.op == '*') {
            auto num = (expr.lhs == unknown) ?
                *evaluate_variable(defs, expr.rhs) :
                *evaluate_variable(defs, expr.lhs);
            auto set_var = (expr.lhs == unknown) ? expr.rhs : expr.lhs;
            return {
                variable_def{set_var, num},
                variable_def{unknown, binary_expression{'/', def.var, set_var}}
            };
        }
    }

    int64_t do_part_2(const var_def_tbl& defs) {

        var_def_tbl solve_for_humn_tbl;
        auto root_def = root_expression_part2(defs);
        solve_for_humn_tbl[root_def.var] = root_def.expr;
        auto definitions = defs;
        definitions.erase("root");
        
        std::string unknown_var = "humn";
        while (!unknown_var.empty()) {
            auto unknown_expr = find_unknown_expression(definitions, unknown_var);
            if (!unknown_expr) {
                if (!definitions.contains(unknown_var)) {
                    unknown_var = {};
                    continue;
                }
                if (unknown_var == root_def.var) {
                    unknown_var = {};
                    continue;
                }
                throw std::runtime_error(
                    "this algorithm in not sophisticated enought to solve this input"
                );
            }
            const auto [var, expr] = *unknown_expr;
            auto new_expressions = solve_for_x({ var,expr }, unknown_var, definitions);
            for (const auto& new_expr : new_expressions) {
                solve_for_humn_tbl[new_expr.var] = new_expr.expr;
            }
            unknown_var = var;
        }

        return *evaluate_variable(solve_for_humn_tbl, "humn");
    }
}

void aoc::day_21(const std::string& title) {
    auto input = file_to_string_vector(input_path(21, 1));
    auto variable_defs = input | rv::transform(str_to_variable_def) |
            rv::transform(
                [](const variable_def& def)->var_def_tbl::value_type {
                    return { def.var,  def.expr };
                }
        ) | r::to<var_def_tbl>();

    std::cout << header(21, title);

    std::cout << "  part 1: " << *evaluate_variable(variable_defs, "root") << "\n";
    std::cout << "  part 2: " << do_part_2(variable_defs) << "\n";
}