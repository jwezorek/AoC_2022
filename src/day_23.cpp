#include "util.h"
#include "days.h"
#include <range/v3/all.hpp>
#include <boost/functional/hash.hpp>
#include <iostream>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <array>
#include <limits>

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

    point_set input_to_elf_locations(const std::vector<std::string>& input) {
        int wd = static_cast<int>(input.front().size());
        int hgt = static_cast<int>(input.size());
        point_set elves;
        for (int y = 0; y < hgt; ++y) {
            for (int x = 0; x < wd; ++x) {
                if (input[y][x] == '#') {
                    elves.insert({ x,y });
                }
            }
        }
        return elves;
    }

    enum direction {
        N = 0, NE, E, SE, S, SW, W, NW
    };

    auto all_directions() {
        return rv::iota(0, 8) |
            rv::transform([](auto i) {return static_cast<direction>(i); });
    }

    point offset_from_dir(direction dir) {
        static std::array<point, 8> offsets = { {
            { 0,-1 }, //N 
            { 1,-1 }, //NE 
            { 1, 0 }, //E 
            { 1, 1 }, //SE 
            { 0, 1 }, //S 
            { -1, 1}, //SW 
            { -1,0 }, //W 
            { -1,-1}  //NW 
        } };
        return offsets[static_cast<int>(dir)];
    }

    auto adjacent_locs(const point& pt) {
        return all_directions() |
            rv::transform(
                [pt](direction dir) {
                    return pt + offset_from_dir(dir);
                }
            );
    }

    bool has_neighbor(const point& pt, const point_set& elves) {
        for (const auto& adj : adjacent_locs(pt)) {
            if (elves.contains(adj)) {
                return true;
            }
        }
        return false;
    }

    auto locs_to_check(const point& pt, direction dir) {
        const static std::unordered_map<direction, std::array<direction, 3>> tbl = {
            { N, {{NW,N,NE}} },
            { E, {{NE,E,SE}} },
            { S, {{SE,S,SW}} },
            { W, {{SW,W,NW}} }
        };
        return tbl.at(dir) |
            rv::transform(
                [pt](direction dir) {
                    return pt + offset_from_dir(dir);
                }
        );
    }

    bool has_neighbor_in_dir(direction dir, const point& pt, const point_set& elves) {
        for (const auto& adj : locs_to_check(pt, dir)) {
            if (elves.contains(adj)) {
                return true;
            }
        }
        return false;
    }

    struct proposal {
        point old_loc;
        point proposed_loc;
    };

    auto proposal_directions(int start_index) {
        const static std::array<direction, 4> prop_dirs = { {
            N,S,W,E
        } };
        return rv::iota(0, 4) |
            rv::transform(
                [start_index](auto i) {
                    return prop_dirs.at((start_index + i) % 4);
                }
        );
    }

    point_set do_one_round(const point_set& elves, int start_index, bool* elf_moved = nullptr) {
        point_set output;
        std::vector<proposal> proposals;
        for (const point& pt : elves) {
            if (!has_neighbor(pt, elves)) {
                output.insert(pt);
                continue;
            }
            bool made_proposal = false;
            for (auto dir : proposal_directions(start_index)) {
                if (!has_neighbor_in_dir(dir, pt, elves)) {
                    made_proposal = true;
                    proposals.push_back({
                        .old_loc = pt,
                        .proposed_loc = pt + offset_from_dir(dir)
                    });
                    break;
                }
            }
            if (!made_proposal) {
                output.insert(pt);
            }
        }
        point_map<int> new_loc_counts;
        for (const auto& prop : proposals) {
            new_loc_counts[prop.proposed_loc]++;
        }
        for (const auto& prop : proposals) {
            if (new_loc_counts.at(prop.proposed_loc) == 1) {
                output.insert(prop.proposed_loc);
                if (elf_moved) {
                    *elf_moved = true;
                }
            } else {
                output.insert(prop.old_loc);
            }
        }
        return output;
    }

    std::tuple<int, int, int, int> bounds(const point_set& pts) {
        int x1 = std::numeric_limits<int>::max();
        int y1 = std::numeric_limits<int>::max();
        int x2 = -std::numeric_limits<int>::max();
        int y2 = -std::numeric_limits<int>::max();
        for (const auto& pt : pts) {
            x1 = (pt.x < x1) ? pt.x : x1;
            y1 = (pt.y < y1) ? pt.y : y1;
            x2 = (pt.x > x2) ? pt.x : x2;
            y2 = (pt.y > y2) ? pt.y : y2;
        }
        return { x1,y1,x2,y2 };
    }

    void print(const point_set& pts) {
        auto [x1, y1, x2, y2] = bounds(pts);
        int wd = x2 - x1;
        int hgt = y2 - y1;
        point origin = { x1,y1 };
        for (int y = 0; y <= hgt; ++y) {
            for (int x = 0; x <= wd; ++x) {
                point pt{ x,y };
                std::cout << (pts.contains(origin + pt) ? '#' : '.');
            }
            std::cout << "\n";
        }
    }

    point_set run_n_rounds(const point_set& pts, int n) {
        auto set = pts;
        int start_dir_index = 0;
        for (int i = 0; i < n; ++i) {
            set = do_one_round(set, start_dir_index);
            start_dir_index = (start_dir_index + 1) % 4;
        }
        return set;
    }

    int run_until_no_elf_moves(const point_set& pts) {
        auto set = pts;
        int start_dir_index = 0;
        int round = 0;
        bool elf_moved = true;
        while (elf_moved) {
            round++;
            elf_moved = false;
            set = do_one_round(set, start_dir_index, &elf_moved);
            start_dir_index = (start_dir_index + 1) % 4;
        }
        return round;
    }

    int count_empty(const point_set& pts) {
        auto [x1, y1, x2, y2] = bounds(pts);
        int wd = x2 - x1;
        int hgt = y2 - y1;
        point origin = { x1,y1 };
        int count = 0;
        for (int y = 0; y <= hgt; ++y) {
            for (int x = 0; x <= wd; ++x) {
                point pt{ x,y };
                count += pts.contains(origin + pt) ? 0 : 1;
            }
        }
        return count;
    }
}

void aoc::day_23(const std::string& title) {
    auto input = file_to_string_vector(input_path(23, 1));
    auto set = input_to_elf_locations(input);

    std::cout << header(23, title);
    std::cout << "  part 1: " << count_empty(run_n_rounds(set,10)) << "\n";
    std::cout << "  part 2: " << run_until_no_elf_moves(set) << "\n";
}