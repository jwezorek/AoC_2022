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
    enum class direction {
        up = 0, right = 1, down = 2, left = 3
    };

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

    point normalize(const point& pt) {
        return { sgn(pt.x), sgn(pt.y) };
    }

    int max_manhattan_delta(const point& pt) {
        return std::max(std::abs(pt.x), std::abs(pt.y));
    }

    using rope = std::vector<point>;
    using movement = std::tuple<direction, int>;

    point move_head(const point& head, direction dir) {
        const static std::unordered_map<direction, point> dir_to_delta = {
            {direction::up, {0,1}},
            {direction::right, {1,0}},
            {direction::down, {0,-1}},
            {direction::left, {-1,0}}
        };
        return head + dir_to_delta.at(dir);
    }

    point move_link(const point& prev, const point& link) {
        auto link_delta = prev - link;
        return (max_manhattan_delta(link_delta) <= 1) ? 
            link :
            link + normalize(link_delta);
    }

    rope move_rope(const rope& r, direction dir) {
        rope new_rope(r.size());
        for (int i = 0; i < r.size(); ++i) {
            new_rope[i] = (i > 0) ?
                move_link(new_rope[i - 1], r[i]):
                move_head(r[0], dir);
        }
        return new_rope;
    }

    movement parse_line_of_input(const std::string& line) {
        const static std::unordered_map<char, direction> letter_to_direction = {
            {'U', direction::up},
            {'R', direction::right},
            {'D', direction::down},
            {'L', direction::left}
        };
        auto pieces = aoc::split(line, ' ');
        return {
            letter_to_direction.at(pieces[0][0]),
            std::stoi(pieces[1])
        };
    }

    int unique_tail_positions(const auto& moves, int length_of_rope) {
        point_set tail_positions;
        auto rope = ::rope(length_of_rope, { 0,0 });
        tail_positions.insert(rope.back());
        for (auto move : moves) {
            auto [dir, dist] = move;
            for (int i = 0; i < dist; ++i) {
                rope = move_rope(rope, dir);
                tail_positions.insert(rope.back());
            }
        }
        return static_cast<int>(tail_positions.size());
    }
}

void aoc::day_9(int day, const std::string& title) {
    auto input = file_to_string_vector(input_path(day, 1));
    auto moves = input | rv::transform(parse_line_of_input) | r::to_vector;

    std::cout << header(day, title);
    std::cout << "  part 1: " << unique_tail_positions(moves, 2) << "\n";
    std::cout << "  part 2: " << unique_tail_positions(moves, 10) << "\n";
}