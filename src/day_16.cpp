#include "util.h"
#include "days.h"
#include <range/v3/all.hpp>
#include <iostream>
#include <functional>
#include <unordered_map>
#include <map>
#include <stack>
#include <boost/functional/hash.hpp>
#include <unordered_set>

namespace r = ranges;
namespace rv = ranges::views;

namespace {

    struct vertex_info {
        std::string label;
        int flow;
        std::vector<std::string> neighbors;
    };

    struct edge {
        int weight;
        int dest;
    };

    struct vertex {
        std::string label;
        int index;
        int flow;
        std::vector<edge> neighbors;
    };

    struct graph {
        int start;
        std::vector<vertex> verts;
    };

    vertex_info parse_line_of_input(const std::string& line) {
        auto pieces = aoc::split(line, ' ');
        return {
            pieces[1],
            std::stoi(aoc::remove_nonnumeric(pieces[4])),
            pieces |
                rv::drop(9) |
                rv::transform(aoc::remove_nonalphabetic) |
                r::to_vector
        };
    }

    graph build_graph(const std::vector<std::string>& input) {
        auto inp = input | rv::transform(parse_line_of_input) | r::to_vector;
        std::unordered_map<std::string, int> label_to_index;
        std::vector<vertex> vertices = rv::enumerate(inp) |
            rv::transform(
                [&label_to_index](const auto& p)->vertex {
                    auto [index, vi] = p;
                    vertex v;
                    v.label = vi.label;
                    v.index = static_cast<int>(index);
                    v.flow = vi.flow;

                    label_to_index[v.label] = v.index;
                    return v;
                }
            ) | r::to_vector;
        for (const auto& vi : inp) {
            auto& v = vertices[label_to_index[vi.label]];
            v.neighbors = vi.neighbors |
                rv::transform(
                    [&label_to_index](const auto& lbl)->edge {
                        return { 1, label_to_index[lbl] };
                    }
                ) | r::to_vector;
        }
        return graph{
            label_to_index["AA"],
            std::move(vertices)
        };
    }

    struct traversal_state {
        uint64_t open_valves;
        int location;
        int minutes_elapsed;
        int total_flow;

        bool operator==(const traversal_state& state) const {
            return open_valves == state.open_valves &&
                location == state.location &&
                minutes_elapsed == state.minutes_elapsed &&
                total_flow == state.total_flow;
        }

        bool is_valve_open(int n) const {
            uint64_t mask = static_cast<uint64_t>(1) << n;
            return open_valves & mask;
        }

        void open_valve(int n) {
            uint64_t mask = static_cast<uint64_t>(1) << n;
            open_valves = open_valves | mask;
        }
    };

    struct state_hasher {
        size_t operator()(const traversal_state& s) const {
            size_t seed = 0;
            boost::hash_combine(seed, s.open_valves);
            boost::hash_combine(seed, s.location);
            boost::hash_combine(seed, s.minutes_elapsed);
            boost::hash_combine(seed, s.total_flow);
            return seed;
        }
    };

    using state_set = std::unordered_set<traversal_state, state_hasher>;

    struct move {
        bool open_valve;
        int dest;
    };

    struct loc_and_valve_state {
        int loc;
        uint64_t valves;

        bool operator==(const loc_and_valve_state& lvs) const {
            return loc == lvs.loc && valves == lvs.valves;
        }
    };

    struct loc_and_valve_state_hasher {
        size_t operator()(const loc_and_valve_state& s) const {
            size_t seed = 0;
            boost::hash_combine(seed, s.loc);
            boost::hash_combine(seed, s.valves);
            return seed;
        }
    };

    using loc_and_valve_map = 
        std::unordered_map<loc_and_valve_state, std::map<int, int>, loc_and_valve_state_hasher>;

    std::vector<move> legal_moves(const graph& g, const traversal_state& state) {
        auto u = state.location;
        bool is_valve_open = state.is_valve_open(u);
        if (!is_valve_open && g.verts[u].flow > 0) {
            return rv::concat(
                rv::single(move{ true, -1 }),
                g.verts[u].neighbors | rv::transform([&](edge e)->move {  return { false, e.dest}; })
            ) | r::to_vector;
        }
        return g.verts[u].neighbors |
            rv::transform([&](auto&& e)->move {  return { false, e.dest }; }) | r::to_vector;
    }

    void augment_flow(const graph& g, traversal_state& state) {
        for (const auto& v : g.verts) {
            if (state.is_valve_open(v.index)) {
                state.total_flow += v.flow;
            }
        }
    }

    traversal_state make_move(const graph& g, const traversal_state& state, const move& m) {
        auto new_state = state;
        augment_flow(g, new_state);
        if (m.open_valve) {
            new_state.open_valve(new_state.location);
        } else {
            new_state.location = m.dest;
        }
        new_state.minutes_elapsed++;
        return new_state;
    }

    using traversal_stack_item = std::tuple<traversal_state, move>;

    using time_table = std::map<int, int>;

    bool check_time_table(time_table& tbl, int time, int val) {
        if (tbl.empty()) {
            tbl[time] = val;
            return true;
        }
        auto i = tbl.lower_bound(time);
        decltype(i) j;
        if (i == tbl.end() || i->first != time) {
            j = std::prev(i);
        } else {
            j = i;
        }
        if (j != tbl.end() && val <= j->second) {
            return false;
        }
        if (i != tbl.end()) {
            tbl.erase(i, tbl.end());
        }
        tbl[time] = val;
        return true;
    }

    int do_traversal(const graph& g) {
        std::stack<traversal_stack_item> stack;
        loc_and_valve_map max_flow_at_time;
        state_set ss;
        traversal_state state{
            .open_valves = 0,
            .location = g.start,
            .minutes_elapsed = 0,
            .total_flow = 0
        };
        for (auto&& m : legal_moves(g, state)) {
            stack.push({ state, m });
        }
        int max_flow = 0;
        max_flow_at_time[loc_and_valve_state{ state.location, 0 }][0] = 0;
        while (!stack.empty()) {
            auto [state, move] = stack.top();
            stack.pop();
            std::cout << stack.size() << " : " << max_flow << " " << state.minutes_elapsed << " " << state.open_valves << " " << state.location << "\n";
            auto new_state = make_move(g, state, move);

            if (ss.contains(new_state)) {
                continue;
            }
            ss.insert(new_state);

            time_table& tbl = max_flow_at_time[loc_and_valve_state{ new_state.location,new_state.open_valves }];
            if (!check_time_table(tbl, new_state.minutes_elapsed, new_state.total_flow)) {
                continue;
            }

            if (new_state.minutes_elapsed == 30) {
                max_flow = std::max(max_flow, new_state.total_flow);
                continue;
            }
            for (auto&& m : legal_moves(g, new_state)) {
                stack.push({ new_state, m });
            }
        }
        return max_flow;
    }

}

/*------------------------------------------------------------------------------------------------*/

void aoc::day_16(const std::string& title) {
    auto input = file_to_string_vector(input_path(16, {}));
    auto g = build_graph(input);

    int foo = do_traversal(g);

    std::cout << header(16, title);
    std::cout << "  part 1: " << foo << "\n";
    std::cout << "  part 2: " << 0 << "\n";
}