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

    using binary_op = std::function<int64_t(int64_t, int64_t)>;
    using var_def_tbl = std::unordered_map<std::string, expression>;

    std::optional<int64_t> maybe_evaluate_variable(const var_def_tbl& defs, const std::string& var){
        const static std::unordered_map<char, binary_op> op_tbl = {
            {'+', [](int64_t lhs, int64_t rhs)->int64_t { return lhs + rhs; }},
            {'-', [](int64_t lhs, int64_t rhs)->int64_t { return lhs - rhs; }},
            {'*', [](int64_t lhs, int64_t rhs)->int64_t { return lhs * rhs; }},
            {'/', [](int64_t lhs, int64_t rhs)->int64_t { return lhs / rhs; }}
        };
        std::stack<int64_t> arg_stack;
        std::stack<std::string> eval_stack;
        eval_stack.push({ var });
        while (!eval_stack.empty()) {
            auto item = eval_stack.top();
            eval_stack.pop();
            if (item.size() == 1) {
                auto func = op_tbl.at(item.front());
                auto arg1 = arg_stack.top();
                arg_stack.pop();
                auto arg2 = arg_stack.top();
                arg_stack.pop();
                arg_stack.push(func(arg1, arg2));
            } else {
                if (!defs.contains(item)) {
                    return {};
                }
                auto val = defs.at(item);
                if (std::holds_alternative<int64_t>(val)) {
                    arg_stack.push(std::get<int64_t>(val));
                } else {
                    auto expr = std::get<binary_expression>(val);
                    eval_stack.push(std::string(1,expr.op));
                    eval_stack.push(expr.lhs);
                    eval_stack.push(expr.rhs);
                }
            }
        }
        return arg_stack.top();
    }

    int64_t evaluate_variable(const var_def_tbl& defs, const std::string& var) {
        auto result = maybe_evaluate_variable(defs, var);
        return result.value();
    }

    std::optional<variable_def> find_unknown_expression(
            const var_def_tbl& definitions, const std::string& variable) {
        for (const auto& var_def : definitions ) {
            const auto [var, expr] = var_def;
            if (std::holds_alternative<binary_expression>(expr)) {
                const auto& bin_expr = std::get<binary_expression>(expr);
                if (bin_expr.lhs == variable || bin_expr.rhs == variable) {
                    return { { var, bin_expr } };
                }
            }
        }
        return {};
    }

    variable_def root_definition(const var_def_tbl& definitions, const std::string unknown) {
        auto root_expr = std::get<binary_expression>(definitions.at("root"));
        auto defs = definitions;
        defs.erase(unknown);
        auto lhs = maybe_evaluate_variable(defs, root_expr.lhs);
        auto rhs = maybe_evaluate_variable(defs, root_expr.rhs);
        return (lhs) ? variable_def{ root_expr.rhs, *lhs } : variable_def{ root_expr.lhs, *rhs };
    }

    std::vector<variable_def> solve_for_x(const variable_def& def, std::string unknown,
            const var_def_tbl& defs) {
        const auto& expr = std::get<binary_expression>(def.expr);
        bool unknown_on_left = (expr.lhs == unknown);
        auto old_var = def.var;
        auto arg_var = unknown_on_left ? expr.rhs : expr.lhs;
        auto arg_value = evaluate_variable(defs, arg_var);
        std::string key = std::string(1, expr.op) + ((unknown_on_left) ? "L" : "R");
        std::unordered_map<std::string, binary_expression> inverse_op = {
            {"-L", { '+', old_var, arg_var} } , {"+L", { '-', old_var, arg_var} },
            {"-R", { '-', arg_var, old_var} } , {"+R", { '-', old_var, arg_var} },
            {"/L", { '*', old_var, arg_var} } , {"*L", { '/', old_var, arg_var} },
            {"/R", { '/', arg_var, old_var} } , {"*R", { '/', old_var, arg_var} }
        };
        return {
            variable_def{arg_var, arg_value},
            variable_def{unknown, inverse_op.at(key) }
        };
    }

    int64_t solve_for_unknown(const var_def_tbl& defs, const std::string& unknown) {

        var_def_tbl solve_for_humn_tbl;
        auto root_def = root_definition(defs, unknown);
        solve_for_humn_tbl[root_def.var] = root_def.expr;
        auto definitions = defs;
        definitions.erase("root");
        
        std::string unknown_var = unknown;
        while (!unknown_var.empty()) {
            auto unknown_expr = find_unknown_expression(definitions, unknown_var);
            if (!unknown_expr) {
                if (unknown_var == root_def.var) {
                    unknown_var = {};
                    continue;
                }
                throw std::runtime_error(
                    "this algorithm in not sophisticated enough to solve this input"
                );
            }
            const auto [var, expr] = *unknown_expr;
            auto new_expressions = solve_for_x({ var,expr }, unknown_var, definitions);
            for (const auto& new_expr : new_expressions) {
                solve_for_humn_tbl[new_expr.var] = new_expr.expr;
            }
            unknown_var = var;
        }

        return evaluate_variable(solve_for_humn_tbl, unknown);
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

    std::cout << "  part 1: " << evaluate_variable(variable_defs, "root") << "\n";
    std::cout << "  part 2: " << solve_for_unknown(variable_defs, "humn") << "\n";
}