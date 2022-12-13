#include "util.h"
#include "days.h"
#include "peglib.h"
#include <range/v3/all.hpp>
#include <iostream>
#include <functional>
#include <memory>
#include <variant>
#include <sstream>
#include <tuple>
#include <optional>

namespace r = ranges;
namespace rv = ranges::views;

/*------------------------------------------------------------------------------------------------*/

namespace {

    struct wrapped_list;
    using list = std::variant<int, std::vector<wrapped_list>>;

    struct wrapped_list {
        list val;
    };

    peg::parser make_parser() {

        std::stringstream ss;
        ss << "ROOT <- Expr\n";
        ss << "Number <- <[0-9]+>\n";
        ss << "Expr <- '[' (Number/Expr)* ']'\n";
        ss << "%whitespace <- [ ,]*\n";

        auto parser = peg::parser(ss.str());
        
        parser["Expr"] = [](const peg::SemanticValues& vs)->list {
            std::vector<wrapped_list> args(vs.size());
            std::transform(vs.begin(), vs.end(), args.begin(),
                [](auto vsi)->wrapped_list {
                    if (vsi.type() == typeid(int)) {
                        return wrapped_list{ std::any_cast<int>(vsi) };
                    } else {
                        return wrapped_list{ std::any_cast<list>(vsi) };
                    }
                }
            );
            return list{ args };
        };

        parser["Number"] = [](const peg::SemanticValues& vs)->int {
            return vs.token_to_number<int>();
        };

        return parser;
    }

    template<class... Ts> struct overloaded : Ts...
    { using Ts::operator()...; };

    template<class... Ts> overloaded(Ts...)->overloaded<Ts...>;

    std::string to_string(const list& list) {
        std::stringstream ss;
        std::visit(
            overloaded{
                [&ss](int val) { ss << val; },
                [&ss](const std::vector<wrapped_list>& lst) {
                    ss << "[ ";
                    for (const auto& itm : lst) {
                        ss << to_string(itm.val) << " ";
                    }
                    ss << "]";
                }
            },
            list
        );
        return ss.str();
    }

    list parse_list(const std::string& str) {
        static auto parser = make_parser();
        list expr;
        bool success = parser.parse(str, expr);
        if (!success) {
            throw std::runtime_error("parsing failure");
        }
        return expr;
    }

    auto to_list_pairs(const std::vector<std::string>& lines) {
        return lines |
            rv::chunk_by(
                [](auto&& lhs, auto&& rhs) { return !lhs.empty() && !rhs.empty(); }
            ) | rv::remove_if(
                [](auto rng) {return rng[0].empty();}
            ) | rv::transform(
                [](auto rng)->std::tuple<list, list> {
                    return {
                        parse_list(rng[0]),
                        parse_list(rng[1])
                    };
                }
            ) | r::to_vector;
    }

    auto to_lists(const std::vector<std::string>& lines) {
        return lines |
            rv::remove_if([](auto&& line) {return line.empty(); }) |
            rv::transform(parse_list) |
            r::to_vector;
    }

    bool is_num(const list& lst) {
        return std::holds_alternative<int>(lst);
    }

    list wrap_in_list(list val) {
        std::vector<wrapped_list> lst = { wrapped_list{val} };
        return lst;
    }

    std::optional<bool> is_well_ordered(const list& lhs, const list& rhs) {
        if (is_num(lhs) && is_num(rhs)) {
            auto left = std::get<int>(lhs);
            auto right = std::get<int>(rhs);
            if (left < right) {
                return true;
            }
            if (left > right) {
                return false;
            }
            return {};
        }

        if (!is_num(lhs) && !is_num(rhs)) {
            const auto& left = std::get<std::vector<wrapped_list>>(lhs);
            const auto& right = std::get<std::vector<wrapped_list>>(rhs);
            auto n = std::min(left.size(), right.size());
            for (size_t i = 0; i < n; ++i) {
                auto result = is_well_ordered(left[i].val, right[i].val);
                if (result) {
                    return result;
                }
            }
            if (left.size() < right.size()) {
                return true;
            }
            if (left.size() > right.size()) {
                return false;
            }
            return {};
        }

        if (is_num(lhs)) {
            return is_well_ordered(wrap_in_list(lhs), rhs);
        }

        if (is_num(rhs)) {
            return is_well_ordered(lhs, wrap_in_list(rhs));
        }

        return {};
    }

    int sum_of_indices_of_well_ordered_pairs(const auto& pairs) {
        return r::accumulate(
            rv::enumerate(pairs) |
                rv::transform(
                    [](auto&& index_pair)->int {
                        const auto& [index, pair] = index_pair;
                        const auto& [lhs, rhs] = pair;
                        auto result = is_well_ordered(lhs, rhs);
                        if (result.has_value() && result.value() == true) {
                            return static_cast<int>(index) + 1;
                        } else {
                            return 0;
                        }
                    }
                ),
            0
        );
    }

    int calculate_decoder_key(const std::vector<std::string>& input) {
        auto lists = to_lists(input);
        lists.emplace_back(parse_list("[[2]]"));
        lists.emplace_back(parse_list("[[6]]"));

        r::sort(lists,
            [](const auto& lhs, const auto& rhs) {
                auto result = is_well_ordered(lhs, rhs);
                return result.has_value() && result.value();
            }
        );

        int divider_packet_product = 1;
        for (const auto& [index, lst] : rv::enumerate(lists)) {
            auto stringified = to_string(lst);
            if (stringified == "[ [ 2 ] ]" || stringified == "[ [ 6 ] ]") {
                divider_packet_product *= static_cast<int>(index) + 1;
            }
        }

        return divider_packet_product;
    }
}

void aoc::day_13(const std::string& title) {
    auto input = file_to_string_vector(input_path(13, 1));
    std::cout << header(13, title);
    auto list_pairs = to_list_pairs(input);
    std::cout << "  part 1: " << sum_of_indices_of_well_ordered_pairs(list_pairs) << "\n";
    std::cout << "  part 2: " << calculate_decoder_key(input) << "\n";
}