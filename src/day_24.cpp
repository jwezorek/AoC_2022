#include "util.h"
#include "days.h"
#include <range/v3/all.hpp>
#include <iostream>
#include <functional>
#include <boost/functional/hash.hpp>
#include <unordered_map>
#include <unordered_set>
#include <array>
#include <numeric>
#include <queue>

namespace r = ranges;
namespace rv = ranges::views;

/*------------------------------------------------------------------------------------------------*/

namespace {

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

    struct point_hash {
        size_t operator()(const point& pt) const {
            size_t seed = 0;
            boost::hash_combine(seed, pt.x);
            boost::hash_combine(seed, pt.y);
            return seed;
        }
    };

    using point_set = std::unordered_set<point, point_hash>;

    enum class direction : uint8_t {
        north = 0,
        east,
        south,
        west,
        none
    };

    std::optional<direction> char_to_direction(char ch) {
        const static std::unordered_map<char, direction> tbl{
            {'^', direction::north },
            {'>', direction::east},
            {'v', direction::south},
            {'<', direction::west}
        };
        return tbl.contains(ch) ?
            std::optional<direction>{tbl.at(ch)} :
            std::optional<direction>{};
    }

    point direction_to_offset(direction dir) {
        const static std::unordered_map<direction, point> tbl{
            { direction::north , {0,-1}},
            { direction::east  , {1,0}},
            { direction::south , {0,1}},
            { direction::west  , {-1,0}},
            { direction::none  , {0,0}}
        };
        return tbl.at(dir);
    }

    point entrance_loc() {
        return { 1,0 };
    }

    point exit_loc(int wd, int hgt) {
        return { wd - 2, hgt - 1 };
    }

    bool is_wall(const point& pt, int wd, int hgt) {
        auto entrance = entrance_loc();
        auto exit = exit_loc(wd, hgt);
        return (pt.x <= 0 || pt.x >= wd - 1 || pt.y <= 0 || pt.y >= hgt - 1) &&
            (pt != entrance) && (pt != exit);
    }

    point wrap_loc(const point& pt, int wd, int hgt) {
        auto new_loc = pt;
        if (new_loc.y == 0) {
            new_loc.y = hgt - 2;
        } else if (new_loc.y == hgt - 1) {
            new_loc.y = 1;
        } else if (new_loc.x == 0) {
            new_loc.x = wd - 2;
        } else if (new_loc.x == wd - 1) {
            new_loc.x = 1;
        }
        return new_loc;
    }

    point wrapped_neighbor_in_direction(const point& pt, direction dir, int wd, int hgt) {
        auto new_loc = pt + direction_to_offset(dir);
        if (is_wall(new_loc, wd, hgt)) {
            new_loc = wrap_loc(new_loc, wd, hgt);
        }
        return new_loc;
    }

    std::optional<point> neighbor_in_direction(const point& pt, direction dir, int wd, int hgt) {
        auto new_loc = pt + direction_to_offset(dir);
        if (is_wall(new_loc, wd, hgt)) {
            return {};
        }
        return new_loc;
    }

    class blizzard_set {
        int wd_;
        int hgt_;
        std::array<point_set, 4> impl_;
    public:

        blizzard_set(int wd = 0, int hgt = 0) :
            wd_(wd),
            hgt_(hgt) 
        {}

        void insert(const point& loc, direction dir) {
            impl_[static_cast<int>(dir)].insert(loc);
        }

        bool contains(const point& pt) const {
            for (const auto& ps : impl_) {
                if (ps.contains(pt)) {
                    return true;
                }
            }
            return false;
        }

        int count(const point& pt) const {
            int n = 0;
            for (const auto& ps : impl_) {
                if (ps.contains(pt)) {
                    ++n;
                }
            }
            return n;
        }

        int width() const {
            return wd_;
        }

        int height() const {
            return hgt_;
        }

        blizzard_set next() const {
            blizzard_set next_set(wd_, hgt_);
            for (int i = 0; i < 4; ++i) {
                auto dir = static_cast<direction>(i);
                for (const auto& pt : impl_[i]) {
                    next_set.insert(
                        wrapped_neighbor_in_direction(pt, dir, wd_, hgt_), 
                        dir
                    );
                }
            }
            return next_set;
        }
    };

    blizzard_set parse_input(const std::vector<std::string>& lines) {
        int hgt = static_cast<int>(lines.size());
        int wd = static_cast<int>(lines.front().size());
        blizzard_set blizz_set(wd, hgt);
        for (int y = 0; y < hgt; ++y) {
            for (int x = 0; x < wd; ++x) {
                auto blizz = char_to_direction(lines[y][x]);
                if (blizz) {
                    blizz_set.insert(point{ x,y }, *blizz );
                }
            }
        }
        return blizz_set;
    }

    using blizzard_atlas = std::vector<blizzard_set>;

    blizzard_atlas make_blizzard_atlas(const blizzard_set& initial) {
        auto n = std::lcm(initial.width()-2, initial.height()-2);
        blizzard_atlas atlas(n);
        atlas[0] = initial;
        for (int i = 1; i < n; ++i) {
            atlas[i] = atlas[i - 1].next();
        }
        return atlas;
    }

    struct state {
        point loc;
        int time;
        std::vector<point> path;

        bool operator==(const state& s) const {
            return loc == s.loc && time == s.time;
        }

        state normalized(int n) const {
            return {
                loc,
                time % n
            };
        }
    };

    struct state_hash {
        size_t operator()(const state& s) const {
            size_t seed = 0;
            boost::hash_combine(seed, s.loc.x);
            boost::hash_combine(seed, s.loc.y);
            boost::hash_combine(seed, s.time);
            return seed;
        }
    };

    using state_set = std::unordered_set<state, state_hash>;

    auto directions() {
        return rv::iota(0, 5) | rv::transform([](int i) {return static_cast<direction>(i); });
    }

    std::optional<point> will_be_empty(const state& s, const blizzard_atlas& atlas, direction dir) {
        int wd = atlas.front().width();
        int hgt = atlas.front().height();
        auto adj = neighbor_in_direction(s.loc, dir, wd, hgt);
        if (!adj) {
            return {};
        }
        if (atlas[(s.time + 1) % atlas.size()].contains(*adj)) {
            return {};
        }
        return *adj;
    }

    auto neighbors_at_place_and_time(const state& s, const blizzard_atlas& atlas) {
        return directions() |
            rv::transform(
                [&](direction dir)->std::optional<point> {
                    return will_be_empty(s, atlas, dir);
                }
            ) | rv::remove_if(
                [](auto&& maybe_pt) {
                    return !maybe_pt.has_value(); 
                }
            ) | rv::transform( 
                [&](auto && maybe_pt)->state { 
                    return {
                        maybe_pt.value(),
                        s.time + 1
                    };
                }
            );
    }

    int min_time_crossing(  const blizzard_atlas& atlas, const point& from, 
                const point& to, int start_time) {
        int atlas_sz = static_cast<int>(atlas.size());
        int wd = atlas.front().width();
        int hgt = atlas.front().height();
        std::queue<state> queue;
        queue.push({ from, start_time });
        state_set visited;
        while (!queue.empty()) {
            auto state = queue.front();
            queue.pop();

            if (state.loc == to) {
                return state.time;
            }

            if (visited.contains(state.normalized(atlas_sz))){
                continue;
            }
            visited.insert(state.normalized(atlas_sz));

            for (auto&& new_state : neighbors_at_place_and_time(state, atlas)) {
                queue.push(new_state);
            }
        }
        return -1;
    }

    int min_time_crossing_back_and_forth(
            const blizzard_atlas& atlas, const point& from, const point& to, int start_time) {

        auto there = min_time_crossing(atlas, from, to, 0);
        auto back = min_time_crossing(atlas, to, from, there);
        auto there_again = min_time_crossing(atlas, from, to, back);

        return there_again;
    }
}

void aoc::day_24(const std::string& title) {
    auto input = file_to_string_vector(input_path(24, 1));
    auto atlas = make_blizzard_atlas(parse_input(input));

    int wd = atlas.front().width();
    int hgt = atlas.front().height();

    std::cout << header(24, title);
    std::cout << "  part 1: " << 
        min_time_crossing(atlas, entrance_loc(), exit_loc(wd,hgt), 0) << "\n";
    std::cout << "  part 2: " << 
        min_time_crossing_back_and_forth(atlas, entrance_loc(), exit_loc(wd, hgt), 0) << "\n";
}