#include "util.h"
#include "days.h"
#include <range/v3/all.hpp>
#include <iostream>
#include <filesystem>
#include <functional>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <sstream>
#include <boost/functional/hash.hpp>

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

    struct point_hasher {
        size_t operator()(const point& pt) const {
            size_t seed = 0;
            boost::hash_combine(seed, pt.x);
            boost::hash_combine(seed, pt.y);
            return seed;
        }
    };

    using point_set = std::unordered_set<point, point_hasher>;

    template <typename T> int sgn(T val) {
        return (T(0) < val) - (val < T(0));
    }

    using rope = std::vector<point>;
    using movement = std::tuple<char, int>;

    point move_head(const point& head, char dir) {
        const static std::unordered_map<char, point> dir_to_delta = {
            {'U', {0,1}}, {'R', {1,0}}, {'D', {0,-1}}, {'L', {-1,0}}
        };
        return head + dir_to_delta.at(dir);
    }

    point move_knot(const point& prev, const point& link) {
        auto delta = prev - link;
        return (std::max(std::abs(delta.x), std::abs(delta.y)) <= 1) ?
            link :
            link + point{ sgn(delta.x), sgn(delta.y) };
    }

    rope move_rope(const rope& r, char direction) {
        auto dummy_head = move_head(move_head(r.front(), direction), direction);
        return rv::concat(rv::single(dummy_head), rv::all(r)) |
            rv::partial_sum(move_knot) | rv::drop(1) | r::to_vector;
    }

    /* 
    * 
    // non-range version below...
    
    rope move_rope(const rope& r, char direction) {
        rope new_rope(r.size());
        for (int i = 0; i < r.size(); ++i) {
            new_rope[i] = (i > 0) ?
                move_link(new_rope[i - 1], r[i]):
                move_head(r[0], direction);
        }
        return new_rope;
    }
    
    */

    movement parse_line_of_input(const std::string& line) {
        auto pieces = aoc::split(line, ' ');
        return {pieces[0][0], std::stoi(pieces[1])};
    }

    int unique_tail_positions(const auto& moves, int length_of_rope) {
        point_set tail_positions;
        auto rope = ::rope(length_of_rope, { 0,0 });
        tail_positions.insert(rope.back());
        for (auto [direction, distance] : moves) {
            for (int i = 0; i < distance; ++i) {
                rope = move_rope(rope, direction);
                tail_positions.insert(rope.back());
            }
        }
        return static_cast<int>(tail_positions.size());
    }
}

void aoc::day_9(const std::string& title) {
    auto input = file_to_string_vector(input_path(9, 1));
    auto moves = input | rv::transform(parse_line_of_input) | r::to_vector;

    std::cout << header(9, title);
    std::cout << "  part 1: " << unique_tail_positions(moves, 2) << "\n";
    std::cout << "  part 2: " << unique_tail_positions(moves, 10) << "\n";
}