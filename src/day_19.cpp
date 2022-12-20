#include "util.h"
#include "days.h"
#include <range/v3/all.hpp>
#include <iostream>
#include <functional>
#include <array>
#include <optional>
#include <stack>
#include <memory>
#include <unordered_set>
#include <sstream>
#include <boost/functional/hash.hpp>
#include <chrono>

namespace r = ranges;
namespace rv = ranges::views;
namespace ch = std::chrono;

/*------------------------------------------------------------------------------------------------*/

namespace {

    enum resource {
        ore = 0,
        clay,
        obsidian,
        geode
    };

    struct cost {
        resource type;
        int amount;
    };

    struct robot {
        int ore_cost;
        std::optional<cost> additional_cost;
        resource output;
    };

    using amount_t = uint8_t;
    using resource_ary = std::array<amount_t, 4>;

    bool can_be_built(const ::robot& robot, const resource_ary& available_resources) {
        if (available_resources[ore] < robot.ore_cost) {
            return false;
        }
        if (!robot.additional_cost.has_value()) {
            return true;
        }
        const cost& additional_cost = robot.additional_cost.value();
        return available_resources[additional_cost.type] >= additional_cost.amount;
    }

    void pay_for_robot(const ::robot& robot, resource_ary& available_resources) {
        available_resources[ore] -= robot.ore_cost;
        if (!robot.additional_cost.has_value()) {
            return;
        }
        const cost& additional_cost = robot.additional_cost.value();
        available_resources[additional_cost.type] -= additional_cost.amount;
    }

    struct blueprint {
        int id;
        std::array<robot,4> robots;

        int max_cost_in_ore() const {
            return r::max(
                robots |
                rv::transform([](auto&& r) {return r.ore_cost; })
            );
        }
    };

    blueprint string_to_blueprint(const std::string& str) {
        auto numbers = aoc::extract_numbers(str);
        blueprint bp;
        bp.id = numbers[0];
        bp.robots[ore] = { numbers[1], {}, ore };
        bp.robots[clay] = { numbers[2], {}, clay };
        bp.robots[obsidian] = { numbers[3], {{clay, numbers[4]}}, obsidian };
        bp.robots[geode] = { numbers[5], {{obsidian, numbers[6]}}, geode };
        return bp;
    }

    std::vector<blueprint> parse_input(const std::vector<std::string>& lines) {
        return lines |
            rv::transform(string_to_blueprint) |
            r::to_vector;
    }

    struct search_state {
        int minute;
        resource_ary num_robots;
        resource_ary rsrc_amounts;
        std::optional<resource> robot_in_production;

        search_state() :
            minute(0),
            num_robots({ {1,0,0,0} }),
            rsrc_amounts({ {0,0,0,0} })
        {}

        bool operator==(const search_state& state) const {
            if (minute != state.minute) {
                return false;
            }
            for (int i = 0; i < 4; ++i) {
                if (num_robots[i] != state.num_robots[i]) {
                    return false;
                }
                if (rsrc_amounts[i] != state.rsrc_amounts[i]) {
                    return false;
                }
            }
            return true;
        }
    };

    struct state_hash {
        size_t operator()(const search_state& state) const {
            size_t seed = 0;
            boost::hash_combine(seed, state.minute);
            for (int i = 0; i < 4; ++i) {
                boost::hash_combine(seed, state.num_robots[i]);
                boost::hash_combine(seed, state.rsrc_amounts[i]);
            }
            return seed;
        }
    };

    using state_set = std::unordered_set<search_state, state_hash>;

    using allocation = std::optional<resource>;

    auto resources() {
        return rv::iota(0, 4) |
            rv::transform(
                [](int i) {
                    return static_cast<resource>(i);
                }
        );
    }

    std::vector<allocation> possible_actions(const blueprint& bp, const search_state& state) {
        std::vector<allocation> allocations = { allocation{} };
        for (auto robot_type : resources()) {
            if (can_be_built(bp.robots[robot_type], state.rsrc_amounts)) {
                if (robot_type == ore && state.num_robots[ore] > bp.max_cost_in_ore()) {
                    continue;
                }
                allocations.push_back({ robot_type });
            }
        }
        return allocations;
    }

    search_state perform_allocation(const blueprint& bp, const search_state& state, allocation a) {
        if (!a.has_value()) {
            return state;
        }
        search_state new_state = state;
        auto new_robot_type = a.value();
        new_state.robot_in_production = new_robot_type;
        pay_for_robot(bp.robots[new_robot_type], new_state.rsrc_amounts);

        return new_state;
    }

    void do_production(search_state& state) {
        for (auto rsrc : resources()) {
            state.rsrc_amounts[rsrc] += state.num_robots[rsrc];
        }
        if (state.robot_in_production) {
            auto robot_type = state.robot_in_production.value();
            state.robot_in_production = {};
            state.num_robots[robot_type]++;
        }
    }

    int maximize_geodes(const blueprint& bp, int duration) {
        std::stack<search_state> stack;
        stack.push({});

        state_set states_seen;
        std::vector<amount_t> max_geodes_per_minute(duration, 0);
        while (!stack.empty()) {
            auto state = stack.top();
            stack.pop();

            if (state.minute > 0) {
                if (states_seen.contains(state)) {
                    continue;
                }
                states_seen.insert(state);

                if (state.rsrc_amounts[geode] < max_geodes_per_minute[state.minute - 1] - 1) {
                    continue;
                }
                max_geodes_per_minute[state.minute - 1] = std::max(state.rsrc_amounts[geode], max_geodes_per_minute[state.minute - 1]);
                if (state.minute == duration) {
                    continue;
                }
            }

            std::vector<allocation> actions = possible_actions(bp, state);
            auto next_states = actions | rv::transform(
                [&bp, &state](auto&& a)->search_state {
                    auto allocated = perform_allocation(bp, state, a);
                    do_production(allocated);
                    allocated.minute++;

                    return allocated;
                }
            );
            for (auto&& next_state : next_states) {
                stack.push(next_state);
            }
        }

        return max_geodes_per_minute[duration - 1];
    }

    int sum_of_quality_level(const std::vector<blueprint>& blueprints, int minute) {
        int sum = 0;
        int i = 0;
        for (const auto& blueprint : blueprints) {
            int geodes = maximize_geodes(blueprint, minute);
            sum += blueprint.id * geodes;
        }
        return sum;
    }

    int product_of_max_geodes(const std::array<blueprint, 3> blueprints, int minute) {
        return maximize_geodes(blueprints[0], minute) *
            maximize_geodes(blueprints[1], minute) *
            maximize_geodes(blueprints[2], minute);
    }
}

void aoc::day_19(const std::string& title) {
    auto input = file_to_string_vector(input_path(19, 1));
    auto blueprints = parse_input(input);

    auto t1 =  ch::high_resolution_clock::now();
    
    std::cout << header(19, title);
    std::cout << "  part 1: " << sum_of_quality_level(blueprints, 24) << "\n";

    std::array<blueprint, 3> ary = { blueprints[0], blueprints[1], blueprints[2] };
    std::cout << "  part 2: " << product_of_max_geodes(ary, 32)  << "\n";

    auto dur = ch::duration_cast<ch::seconds>(ch::high_resolution_clock::now() - t1);
    std::cout << "time: " << dur.count() << " sec.";
}