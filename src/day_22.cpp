#include "util.h"
#include "days.h"
#include <range/v3/all.hpp>
#include <iostream>
#include <functional>
#include <array>

namespace r = ranges;
namespace rv = ranges::views;

/*------------------------------------------------------------------------------------------------*/

namespace {

    struct instruction {
        bool go_forward;
        int amount;
    };

    using instructions = std::vector<instruction>;

    struct point {
        int x;
        int y;

        bool operator==(const point& p) const {
            return x == p.x && y == p.y;
        }
    };

    point operator+(const point& lhs, const point& rhs) {
        return  { lhs.x + rhs.x, lhs.y + rhs.y };
    }

    point operator-(const point& lhs, const point& rhs) {
        return  { lhs.x - rhs.x, lhs.y - rhs.y };
    }

    point operator*(int lhs, const point& rhs) {
        return {
            lhs * rhs.x,
            lhs * rhs.y
        };
    }

    const std::array<point, 4> k_directions = { {
        {1,0},
        {0,1},
        {-1,0},
        {0,-1}
    } };

    constexpr char k_empty = '.';
    constexpr char k_wall = '#';
    constexpr char k_outer_space = ' ';
    constexpr int k_right = 0;
    constexpr int k_down = 1;
    constexpr int k_left = 2;
    constexpr int k_up = 3;

    class grid {
        std::vector<std::vector<char>> impl_;
        std::vector <std::tuple<int, int>> horz_bounds_;
        std::vector <std::tuple<int, int>> vert_bounds_;
        int width_;

        static std::tuple<int, int> get_bounds(auto row_or_col) {
            auto padded_row = rv::concat(rv::single(k_outer_space), row_or_col);
            auto chunks = padded_row | 
                rv::chunk_by(
                    [](char lhs, char rhs) {
                        return (lhs == k_outer_space && rhs == k_outer_space) ||
                            (lhs != k_outer_space && rhs != k_outer_space);
                    }
                ) | rv::transform(
                    [](auto rng) {
                        return r::distance(rng);
                    }
                ) | r::to_vector;
            // -1 and -2 below are because we padded the left side and
            // and because we want "inclusive" ranges.
            return { chunks[0] - 1, chunks[0] + chunks[1] - 2};
        }

        auto column(int col) const {
            return rv::iota(0, height()) |
                rv::transform(
                    [this,col](int row)->char {
                        return tile({ col,row });
                    }
                );
        }

        auto columns() const {
            int wd = width();
            return rv::iota(0, wd) | 
                rv::transform(
                    [&](int col) {return column(col); }
                );
        }

    public:

        grid(auto lines) :
            impl_(lines |
                rv::transform([](auto&& str)->std::vector<char> {return str | r::to_vector; }) |
                r::to_vector
            )
        {
            width_ = r::max(
                lines | rv::transform([](auto&& str) {return static_cast<int>(str.size()); })
            );

            horz_bounds_ = impl_ |
                rv::transform(
                    [](auto&& row) {return get_bounds(rv::all(row)); }
            ) | r::to_vector;

            vert_bounds_ = columns() |
                rv::transform(
                    [](auto col) {return get_bounds(col); }
            ) | r::to_vector;

        }

        point starting_loc() const {
            return {
                std::get<0>(horz_bounds_.front()),
                0
            };
        }

        char tile(const point& loc) const {
            if (loc.y < 0 || loc.y >= height()) {
                return k_outer_space;
            }
            auto&& row = impl_[loc.y];
            if (loc.x < 0 || loc.x >= static_cast<int>(row.size())) {
                return k_outer_space;
            }
            return row[loc.x];
        }

        point neighbor(const point& loc, int direction) const {
            auto neighbor_pt = loc + k_directions[direction];
            auto [left, right] = horz_bounds_[loc.y];
            if (neighbor_pt.x > right) {
                neighbor_pt.x = left;
            } else if (neighbor_pt.x < left) {
                neighbor_pt.x = right;
            }
            auto [top, bottom] = vert_bounds_[loc.x];
            if (neighbor_pt.y > bottom) {
                neighbor_pt.y = top;
            } else if (neighbor_pt.y < top) {
                neighbor_pt.y = bottom;
            }
            return neighbor_pt;
        }

        int height() const {
            return static_cast<int>(impl_.size());
        }

        int width() const {
            return width_;
        }

        void track(point pt) {
            impl_[pt.y][pt.x] = '@';
        }

        void print() const {
            for (int y = 0; y < height(); ++y) {
                for (int x = 0; x < width(); ++x) {
                    std::cout << tile({ x,y });
                }
                std::cout << "\n";
            }
        }
    };

    std::tuple<grid, instructions> parse_input(const std::vector<std::string>& lines) {
        ::grid grid(lines | rv::take_while([](auto&& str) {return !str.empty(); }));
        auto instructs = lines.back() |
            rv::chunk_by(
                [](char lhs, char rhs)->bool {
                    return std::isdigit(lhs) && std::isdigit(rhs);
                }
            ) |
            rv::transform(
                [](auto rng)->instruction {
                    bool is_forward = std::isdigit(rng[0]);
                    int amt = (is_forward) ? std::stoi(rng | r::to<std::string>()) : 
                        ((rng[0] == 'L') ? -1 : 1);
                    return { is_forward, amt };
                }
            ) | r::to_vector;
         return { std::move(grid), std::move(instructs) };
    }

    struct state {
        point loc;
        int dir;
    };

    state follow_instruction( grid& grid, const instruction& inst, const state& state) {
        ::state s = state;
        if (inst.go_forward) {
            auto tile = k_empty;
            int amt = inst.amount;
            while (grid.tile(grid.neighbor(s.loc, s.dir)) != k_wall && --amt >= 0) {
                s.loc = grid.neighbor(s.loc, s.dir);
            }
        } else {
            s.dir += inst.amount;
            if (s.dir < 0) {
                s.dir = 3;
            } else if (s.dir >= 4) {
                s.dir = 0;
            }
        }
        return s;
    }

    state follow_instructions( grid& grid, const instructions& insts) {
        state state{ grid.starting_loc(), 0 };
        grid.track(state.loc);
        for (const auto& inst : insts) {
            state = follow_instruction(grid, inst, state);
        }
        return state;
    }

    int do_part_1(grid& grid, const instructions& insts) {
        auto end_state = follow_instructions(grid, insts);
        end_state.loc = end_state.loc + point{ 1,1 };

        return 1000 * end_state.loc.y + 4 * end_state.loc.x + end_state.dir;
    }
}

void aoc::day_22(const std::string& title) {
    auto input = file_to_string_vector(input_path(22, 1));
    auto [grid, instructions] = parse_input(input);

    std::cout << header(22, title);
    std::cout << "  part 1: " << do_part_1(grid, instructions) << "\n";
    std::cout << "  part 2: " << 0 << "\n";
}