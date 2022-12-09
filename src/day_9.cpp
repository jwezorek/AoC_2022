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
    };

    point operator+(const point& lhs, const point& rhs) {
        return  { lhs.x + rhs.x, lhs.y + rhs.y };
    }

    point operator-(const point& lhs, const point& rhs) {
        return  { lhs.x - rhs.x, lhs.y - rhs.y };
    }

    struct hash_point {
        size_t operator()(const point& pt) const {
            size_t seed = 0;
            boost::hash_combine(seed, pt.x);
            boost::hash_combine(seed, pt.y);
            return seed;
        }
    };

    struct point_equal {
        bool operator()(const point& p1, const point& p2) const {
            return p1.x == p2.x && p1.y == p2.y;
        }
    };

    using point_set = std::unordered_set<point, hash_point, point_equal>;

    template <typename T> int sgn(T val) {
        return (T(0) < val) - (val < T(0));
    }

    point normalize(const point& pt) {
        return {
            sgn(pt.x),
            sgn(pt.y)
        };
    }

    int max_delta(const point& pt) {
        return std::max(std::abs(pt.x), std::abs(pt.y));
    }

    struct rope {
        point head;
        point tail;
    };

    using movement = std::tuple<direction, int>;

    std::string draw_rope(const rope& r, int min_x, int min_y, int max_x, int max_y) {
        std::stringstream ss;
        for (auto y = max_y-1; y >= min_y; --y) {
            for (auto x = min_x; x < max_x; ++x) {
                char ch = '.';
                if (x == 0 && y == 0) {
                    ch = 's';
                }
                if (x == r.tail.x && y == r.tail.y) {
                    ch = 'T';
                }
                if (x == r.head.x && y == r.head.y) {
                    ch = 'H';
                }
                ss << ch;
            }
            ss << "\n";
        }
        return ss.str();
    }

    rope move_rope(const rope& r, direction dir) {
        const static std::unordered_map<direction, point> dir_to_delta = {
            {direction::up, {0,1}},
            {direction::right, {1,0}},
            {direction::down, {0,-1}},
            {direction::left, {-1,0}}
        };

        auto new_head = r.head + dir_to_delta.at(dir);
        auto tail_delta = new_head - r.tail;
        if (max_delta(tail_delta) <= 1) {
            return {
                new_head,
                r.tail
            };
        }
        auto new_tail = r.tail + normalize(tail_delta);

        return {
            new_head,
            new_tail
        };
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

    int unique_tail_positions(const auto& moves) {
        point_set tail_positions;
        ::rope rope = { {0,0},{0,0} };
        tail_positions.insert(rope.tail);
        for (auto move : moves) {
            auto [dir, dist] = move;
            for (int i = 0; i < dist; ++i) {
                rope = move_rope(rope, dir);
                tail_positions.insert(rope.tail);
            }
        }
        return static_cast<int>(tail_positions.size());
    }
}



void aoc::day_9(int day, const std::string& title) {
    auto input = file_to_string_vector(input_path(day, 1));
    /*
    std::vector<std::string> input = {
        "R 4",
        "U 4",
        "L 3",
        "D 1",
        "R 4",
        "D 1",
        "L 5",
        "R 2"
    };
    */
    auto moves = input | rv::transform(parse_line_of_input) | r::to_vector;

    /*
    ::rope rope = { {0,0},{0,0} };

    std::cout << draw_rope(rope, 0, 0, 6, 6) << "\n";
    for (auto move : moves) {
        auto [dir, dist] = move;
        for (int i = 0; i < dist; ++i) {
            rope = move_rope(rope, dir);
            std::cout << draw_rope(rope, 0, 0, 6, 6) << "\n";
            std::getchar();
        }
    }
    */

    std::cout << header(day, title);
    std::cout << "  part 1: " << unique_tail_positions(moves) << "\n";
    std::cout << "  part 2: " << 0 << "\n";
}