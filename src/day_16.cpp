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

    int length_of_shortest_path(const graph& g, int src, int dest) {
        std::unordered_map<int, int> distance;
        std::function<void(int, int)> dfs;
        dfs = [&](int u, int depth)->void {
            if (distance.contains(u)) {
                if (depth < distance[u]) {
                    distance[u] = depth;
                }
                return;
            }
            distance[u] = depth;
            if (u == dest) {
                return;
            }
            for (int v : g.verts[u].neighbors |  rv::transform(  [](auto&& e) { return e.dest;  } )) {
                dfs(v, depth + 1);
            }
            return;
        };
        dfs(src, 0);
        return distance[dest];
    }

    std::vector<std::vector<int>> shortest_path_lengths(const graph& g) {
        int n = static_cast<int>(g.verts.size());
        std::vector<std::vector<int>> tbl(n, std::vector<int>(n, 0));
        for (auto [u, v] : rv::cartesian_product(rv::iota(0, n), rv::iota(0, n))) {
            tbl[u][v] = length_of_shortest_path(g, u, v);
            tbl[v][u] = tbl[u][v];
        }
        return tbl;
    }

    graph build_weighted_graph(const graph& g, std::vector<std::vector<int>>& dist) {
        std::unordered_map<int, int> new_index_to_old_index =
            rv::enumerate(
                g.verts |
                rv::remove_if(
                    [&](auto&& u) {
                        return u.flow == 0 && u.index != g.start;
                    }
                )
            ) | rv::transform(
                [](auto&& i_u)->std::unordered_map<int, int>::value_type {
                    auto [i, u] = i_u;
                    return { static_cast<int>(i), u.index };
                }
            ) | r::to< std::unordered_map<int, int>>();
        int n = static_cast<int>(new_index_to_old_index.size());

        graph weighted_graph;
        for (int new_index : rv::iota(0, n)) {
            std::vector<edge> edges;
            int old_index = new_index_to_old_index[new_index];
            for (int v : rv::iota(0, n)) {
                if (v == new_index) {
                    continue;
                }
                if (g.verts[new_index_to_old_index[v]].flow == 0) {
                    continue;
                }
                edges.push_back( 
                    edge {
                        dist[old_index][new_index_to_old_index[v]],
                        v
                    }
                );
            }
            const auto& old_vert = g.verts[old_index];
            if (old_vert.label == "AA") {
                weighted_graph.start = new_index;
            }
            weighted_graph.verts.push_back(
                vertex{
                    .label = old_vert.label,
                    .index = new_index,
                    .flow = old_vert.flow,
                    .neighbors = edges
                }
            );
        }
        return weighted_graph;
    }

    struct traversal_state {
        uint64_t open_valves;
        int location;
        int elephant_location;
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

    std::vector<edge> neighbors(const graph& g, const traversal_state& state, int max_time) {
        auto u = state.location;
        return g.verts[u].neighbors |
            rv::remove_if(
                [&](auto&& e)->bool {
                    int v = e.dest;
                    if (g.verts[v].flow == 0) {
                        return true;
                    }
                    if (state.is_valve_open(v)) {
                        return true;
                    }
                    if (state.minutes_elapsed + e.weight >= max_time) {
                        return true;
                    }
                    return false;
                }
        ) | r::to_vector;
    }

    int current_flow(const graph& g, const traversal_state& state) {
        int total_flow = 0;
        for (const auto& v : g.verts) {
            if (state.is_valve_open(v.index)) {
                total_flow += v.flow;
            }
        }
        //std::cout << "total flow: " << total_flow << "\n";
        return total_flow;
    }

    traversal_state make_move(const graph& g, const traversal_state& state, const edge& e) {
        traversal_state new_state = state;
        auto [distance, dest] = e;
        int elapsed_time = distance + 1;
        new_state.minutes_elapsed += elapsed_time;
        new_state.total_flow += elapsed_time * current_flow(g, state);
        new_state.open_valve(dest);
        new_state.location = dest;
        return new_state;
    }

    using traversal_stack_item = std::tuple<traversal_state, edge>;

    int do_traversal(const graph& g, int max_time) {
        std::stack<traversal_stack_item> stack;
        state_set ss;
        traversal_state state{
            .open_valves = 0,
            .location = g.start,
            .minutes_elapsed = 0,
            .total_flow = 0
        };
        for (auto&& e : neighbors(g, state, max_time)) {
            stack.push({ state, e });
        }
        int max_flow = 0;
        while (!stack.empty()) {
            auto [state, e] = stack.top();
            stack.pop();
            //std::cout << stack.size() << " : " << max_flow << " " << state.minutes_elapsed << " " << state.open_valves << " " << state.location << "\n";
            auto new_state = make_move(g, state, e);

            if (new_state.minutes_elapsed == max_time) {
                //std::cout << "foo:" << new_state.minutes_elapsed << "\n";
                max_flow = std::max(max_flow, new_state.total_flow);
                continue;
            }

            auto neigh = neighbors(g, new_state, max_time);
            if (neigh.empty()) {
                auto flow = new_state.total_flow + (max_time - new_state.minutes_elapsed) * current_flow(g, new_state);
                max_flow = std::max(max_flow, flow);
                continue;
            }
            for (auto&& m : neigh) {
                stack.push({ new_state, m });
            }
        }
        return max_flow;
    }

    using traversal_stack_item_with_elphant = std::tuple<traversal_state, edge, edge>;
    /*
    int do_traversal_with_elephant(const graph& g, int max_time) {
        std::stack<traversal_stack_item_with_elphant> stack;
        state_set ss;
        traversal_state state{
            .open_valves = 0,
            .location = g.start,
            .elephant_location = g.start,
            .minutes_elapsed = 0,
            .total_flow = 0
        };
        for (auto&& e : neighbors(g, state, max_time)) {
            stack.push({ state, e });
        }
        int max_flow = 0;
        while (!stack.empty()) {
            auto [state, e] = stack.top();
            stack.pop();
            //std::cout << stack.size() << " : " << max_flow << " " << state.minutes_elapsed << " " << state.open_valves << " " << state.location << "\n";
            auto new_state = make_move(g, state, e);

            if (new_state.minutes_elapsed == max_time) {
                //std::cout << "foo:" << new_state.minutes_elapsed << "\n";
                max_flow = std::max(max_flow, new_state.total_flow);
                continue;
            }

            auto neigh = neighbors(g, new_state, max_time);
            if (neigh.empty()) {
                auto flow = new_state.total_flow + (max_time - new_state.minutes_elapsed) * current_flow(g, new_state);
                max_flow = std::max(max_flow, flow);
                continue;
            }
            for (auto&& m : neigh) {
                stack.push({ new_state, m });
            }
        }
        return max_flow;
    }
    */
}

/*------------------------------------------------------------------------------------------------*/

void aoc::day_16(const std::string& title) {
    auto input = file_to_string_vector(input_path(16, 1));
    auto g = build_graph(input);

    auto tbl = shortest_path_lengths(g);
    g = build_weighted_graph(g, tbl);

    std::cout << header(16, title);
    std::cout << "  part 1: " << do_traversal(g, 30) << "\n";
    std::cout << "  part 2: " << 0 << "\n";
}