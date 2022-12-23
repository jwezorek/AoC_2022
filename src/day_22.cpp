#include "util.h"
#include "days.h"
#include <range/v3/all.hpp>
#include <iostream>
#include <functional>
#include <array>
#include <boost/functional/hash.hpp>
#include <unordered_set>
#include <unordered_map>

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

    struct point_hash {
        size_t operator()(const point& pt) const {
            size_t seed = 0;
            boost::hash_combine(seed, pt.x);
            boost::hash_combine(seed, pt.y);
            return seed;
        }
    };

    using point_set = std::unordered_set<point, point_hash>;

    template<typename T>
    using point_map = std::unordered_map<point, T, point_hash>;

    const std::array<point, 4> k_directions = { {
        {1,0}, // >
        {0,1}, // V
        {-1,0},// <
        {0,-1} // ^
    } };

    constexpr char k_empty = '.';
    constexpr char k_wall = '#';
    constexpr char k_outer_space = ' ';
    constexpr int k_right = 0;
    constexpr int k_down = 1;
    constexpr int k_left = 2;
    constexpr int k_up = 3;

    struct state {
        point loc;
        int dir;

        state turn_right() const {
            int new_dir = dir;
            ++new_dir;
            new_dir = new_dir % 4;
            return { loc, new_dir };
        }

        state turn_left() const {
            int new_dir = dir;
            --new_dir;
            new_dir = (new_dir < 0) ? 3 : new_dir;
            return { loc, new_dir };
        }

        state go_forward() const {
            auto new_loc = loc + k_directions[dir];
            return { new_loc,dir };
        }
    };

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

        int min_cross_section() const {
            return r::min(
                rv::concat(horz_bounds_, vert_bounds_) |
                    rv::transform(
                        [](auto&& tup)->int {
                            auto [u, v] = tup;
                            return v - u + 1;
                        }
                    )
            );
        }

        void print() const {
            for (auto&& row : impl_) {
                for (char ch : row) {
                    std::cout << ch;
                }
                std::cout << "\n";
            }
        }

        point starting_loc() const {
            return {
                std::get<0>(horz_bounds_.front()),
                0
            };
        }

        void set(const point& loc, char ch) {
            impl_[loc.y][loc.x] = ch;
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

        point wrapped_neighbor(const point& loc, int direction) const {
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

        state neighbor(const state& state) const {
            auto neighbor_pt = state.loc + k_directions[state.dir];
            return { neighbor_pt, state.dir };
        }

        point left_neighbor(const state& s) const {
            auto state = s.turn_left();
            return neighbor(state).loc;
        }

        int height() const {
            return static_cast<int>(impl_.size());
        }

        int width() const {
            return width_;
        }

        bool visited_or_space(const point& pt, const point_set& visited) const {
            return visited.contains(pt) || tile(pt) == k_outer_space;
        }

        int probe_forward(const state& s, const point_set& visited) const {
            auto state = s;
            int count = 0;
            while (!visited_or_space(state.go_forward().loc, visited) && visited_or_space(left_neighbor(state), visited)){
                count++;
                state = state.go_forward();
            }
            return count;
        }

        int probe_turn(const state& s, const point_set& visited) const {
            if (!visited_or_space(left_neighbor(s), visited)) {
                return -1;
            }
            return 1;
        }

        instructions spiral_traversal()  {
            state s{ starting_loc(), k_right };
            point_set visited;
            int distance = 0;
            instructions inst;
            do {
                distance = probe_forward(s, visited);
                inst.push_back(instruction{ true, distance });
                for (int i = 0; i < distance; ++i) {
                    visited.insert(s.loc);
                    s = neighbor(s);
                }
                int turn = probe_turn(s, visited);
                s = (turn == 1) ? s.turn_right() : s.turn_left();
                inst.push_back({ false, turn });

            } while (distance > 0);
            return inst;
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

    state follow_instructions( grid& grid, const instructions& insts) {
        state s{ grid.starting_loc(), 0 };
        for (const auto& inst : insts) {
            if (inst.go_forward) {
                auto tile = k_empty;
                int amt = inst.amount;
                while (grid.tile(grid.wrapped_neighbor(s.loc, s.dir)) != k_wall && --amt >= 0) {
                    s.loc = grid.wrapped_neighbor(s.loc, s.dir);
                }
            } else {
                if (inst.amount > 0) {
                    s = s.turn_right();
                } else {
                    s = s.turn_left();
                }
            }
        }
        return s;
    }

    int do_part_1(grid& grid, const instructions& insts) {
        auto end_state = follow_instructions(grid, insts);
        end_state.loc = end_state.loc + point{ 1,1 };

        return 1000 * end_state.loc.y + 4 * end_state.loc.x + end_state.dir;
    }

    int zero(point pt, int dim) {
        return 0;
    }

    int get_x(point pt, int dim) {
        return pt.x;
    }

    int get_max(point pt, int dim) {
        return dim - 1;
    }

    int get_y(point pt, int dim) {
        return pt.y;
    }

    int inv_x(point pt, int dim) {
        return dim - pt.x - 1;
    }

    int inv_y(point pt, int dim) {
        return dim - pt.y - 1;
    }

    using trans_coord_fn = std::function<int(point, int)>;

    struct transition {
        int face;
        int new_dir;
        trans_coord_fn x_fn;
        trans_coord_fn y_fn;
    };

    constexpr int k_top = 0;
    constexpr int k_north = 1;
    constexpr int k_east = 2;
    constexpr int k_south = 3;
    constexpr int k_west = 4;
    constexpr int k_bottom = 5;

    struct cube_point {
        int face;
        point loc;

        bool operator==(const cube_point& pt) const {
            return face == pt.face && loc == pt.loc;
        }
    };

    struct cube_point_hash {
        size_t operator()(const cube_point& cpt) const {
            size_t seed = 0;
            boost::hash_combine(seed, cpt.face);
            boost::hash_combine(seed, cpt.loc.x);
            boost::hash_combine(seed, cpt.loc.y);
            return seed;
        }
    };

    template<typename T>
    using cube_loc_map = std::unordered_map<cube_point, T, cube_point_hash>;

    struct cube_state {
        cube_point loc;
        int dir;

        cube_state turn_right() const {
            int new_dir = dir;
            ++new_dir;
            new_dir = new_dir % 4;
            return { loc, new_dir };
        }

        cube_state turn_left() const {
            int new_dir = dir;
            --new_dir;
            new_dir = (new_dir < 0) ? 3 : new_dir;
            return { loc, new_dir };
        }

    };

    char debug(int n) {
        static std::vector<char> chars;
        if (chars.empty()) {
            chars.resize(62);
            auto iter = chars.begin();
            for (int i = 0; i < 10; ++i) {
                *iter++ = i + '0';
            }
            for (int i = 0; i < 26; ++i) {
                *iter++ = i + 'a';
            }
            for (int i = 0; i < 26; ++i) {
                *iter++ = i + 'A';
            }
        }
        return chars[n % 62];
    }

    class cube_grid {
        std::array<std::vector<std::vector<char>>, 6> faces_;
        std::array<std::array<transition, 4>, 6> face_map_;
        std::array<point, 6> unfolded_;
        int dim_;
        cube_loc_map<point> cube_pt_to_flat_pt_;
        point_map<int> cube_dir_to_flat_dir_;

        void populate_cube_dir_to_flat_dir(const cube_state& cs, const state& s) {
            if (cube_dir_to_flat_dir_.contains({ cs.loc.face,cs.dir })) {
                return;
            }
            int cs_dir = cs.dir;
            int s_dir = s.dir;
            for (int i = 0; i < 4; ++i) {
                cube_dir_to_flat_dir_[{cs.loc.face, cs_dir}] = s_dir;
                cs_dir = (cs_dir + 1) % 4;
                s_dir = (s_dir + 1) % 4;
            }
        }

        void copy_grid( ::grid& grid) {
            auto spiral = grid.spiral_traversal();
            state flat_state{ {grid.starting_loc()}, k_right };
            cube_state cub_state{ starting_loc(), k_right };
            
            for (const auto& inst : spiral) {
                if (inst.go_forward) {
                    for (int i = 0; i < inst.amount; ++i) {
                        populate_cube_dir_to_flat_dir(cub_state, flat_state);
                        set(cub_state.loc, grid.tile(flat_state.loc));
                        cube_pt_to_flat_pt_[cub_state.loc] = flat_state.loc;
                        flat_state = flat_state.go_forward();
                        cub_state = neighbor(cub_state);
                    }
                } else {
                    if (inst.amount > 0) {
                        flat_state = flat_state.turn_right();
                        cub_state = cub_state.turn_right();
                    } else {
                        flat_state = flat_state.turn_left();
                        cub_state = cub_state.turn_left();
                    }
                }
            }
            set(cub_state.loc, grid.tile(flat_state.loc));
            cube_pt_to_flat_pt_[cub_state.loc] = flat_state.loc;
        }

    public:
        cube_grid( grid& g) : dim_(g.min_cross_section()) {
            for (auto& face : faces_) {
                face = std::vector<std::vector<char>>(
                    dim_,
                    std::vector<char>(dim_, k_empty)
                );
            }
            face_map_ = std::array<std::array<transition, 4>, 6>{ {
                {{ {k_east, k_right, zero, get_y}, {k_south, k_down, get_x, zero}, {k_west, k_left, get_max, get_y}, {k_north, k_up, get_x, get_max} }}, //top
                {{ {k_east, k_down, inv_y, zero},{k_top, k_down, get_x, zero},{k_west, k_down, get_y, zero},{k_bottom, k_up, get_x, get_max} }}, //north
                {{ {k_bottom, k_left, get_max, inv_y},{k_south,k_left, get_max, get_x},{k_top, k_left, get_max, get_y},{k_north, k_left, get_max, inv_x} }}, //east
                {{ {k_east,k_up,get_y,get_max},{k_bottom,k_down,get_x,zero},{k_west,k_up,inv_y,get_max},{k_top,k_up,get_x,get_max} }}, //south
                {{ {k_top,k_right,zero,get_y},{k_south,k_right,zero,inv_x},{k_bottom,k_right,zero,inv_y},{k_north,k_right,zero,get_x} }}, //west
                {{ {k_east, k_left, get_max, inv_y},{k_north,k_down,get_x,zero},{k_west,k_right,zero,inv_y},{k_south,k_up,get_x,get_max} }}, //bottom
                }};
            unfolded_ = std::array<point, 6>{{
                {1, 1}, { 1,0 }, { 2,1 }, { 1,2 }, { 0,1 },{1,3}
            }};
            copy_grid(g);
        }

        point to_flat_point(const cube_point& cp) const {
            return cube_pt_to_flat_pt_.at(cp);
        }

        state to_flat_state(const cube_state& cs) const {
            auto pt = cube_pt_to_flat_pt_.at(cs.loc);
            auto dir = cube_dir_to_flat_dir_.at({cs.loc.face, cs.dir});
            return { pt,dir };
        }

        cube_point starting_loc() const {
            return { k_top, {0,0} };
        }

        cube_state neighbor(const cube_state& s) const {
            auto pt = s.loc.loc;
            auto face = s.loc.face;
            auto new_pt = pt + k_directions[s.dir];
            const transition* trans = nullptr;
            if (new_pt.x < 0) {
                trans = &face_map_[face][k_left];
            } else if (new_pt.x == dim_) {
                trans = &face_map_[face][k_right];
            } else if (new_pt.y < 0) {
                trans = &face_map_[face][k_up];
            } else if (new_pt.y == dim_) {
                trans = &face_map_[face][k_down];
            }
            if (!trans) {
                return { {face, new_pt}, s.dir };
            }
            return { 
                cube_point{
                    trans->face,
                    point{ trans->x_fn(pt, dim_), trans->y_fn(pt,dim_) }
                },
                trans->new_dir
            };
        }

        char tile(const cube_point& pt) const {
            return faces_[pt.face][pt.loc.y][pt.loc.x];
        }

        void set(const cube_point& pt, char ch) {
            faces_[pt.face][pt.loc.y][pt.loc.x] = ch;
        }

        void print() const {
            std::vector<std::vector<char>> g(dim_ * 4, std::vector<char>(dim_ * 3, ' '));
            for (int face = 0; face < 6; ++face) {
                point orig = { unfolded_[face].x * dim_, unfolded_[face].y * dim_ };
                for (int y = 0; y < dim_; ++y) {
                    for (int x = 0; x < dim_; ++x) {
                        g[y + orig.y][x + orig.x] = faces_[face][y][x];
                    }
                }
            }
            for (auto&& row : g) {
                for (char t : row) {
                    std::cout << t;
                }
                std::cout << "\n";
            }
        }
    };

    cube_state follow_instructions(cube_grid& grid, const instructions& insts) {
        cube_state s{ grid.starting_loc(), 0 };
        for (const auto& inst : insts) {
            if (inst.go_forward) {
                auto tile = k_empty;
                int amt = inst.amount;
                while (grid.tile(grid.neighbor(s).loc) != k_wall && --amt >= 0) {
                    s = grid.neighbor(s);
                }
            } else {
                if (inst.amount > 0) {
                    s = s.turn_right();
                } else {
                    s = s.turn_left();
                }
            }
        }
        return s;
    }

    int do_part_2(cube_grid& grid, const instructions& insts) {
        auto end_cube_state = follow_instructions(grid, insts);
        auto end_state = grid.to_flat_state(end_cube_state);
        end_state.loc = end_state.loc + point{ 1,1 };

        return 1000 * end_state.loc.y + 4 * end_state.loc.x + end_state.dir;
    }
}

void aoc::day_22(const std::string& title) {
    auto input = file_to_string_vector(input_path(22, 1));
    auto [grid, instructions] = parse_input(input);

    std::cout << header(22, title);
    std::cout << "  part 1: " << do_part_1(grid, instructions) << "\n";
    cube_grid c_grid(grid);
    std::cout << "  part 2: " << do_part_2(c_grid, instructions) << "\n";
}