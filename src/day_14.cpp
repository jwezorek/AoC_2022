#include "util.h"
#include "days.h"
#include <range/v3/all.hpp>
#include <iostream>
#include <functional>
#include <sstream>

namespace r = ranges;
namespace rv = ranges::views;
/*------------------------------------------------------------------------------------------------*/

namespace {

    struct point {
        int x, y;
    };

    enum class tile {
        empty,
        wall,
        sand
    };

    class grid {
        std::vector<std::vector<tile>> ary_;
        int min_x_;
    public:
        grid(int x1, int x2, int y2) : 
            min_x_(x1),
            ary_(y2+1, std::vector<tile>(x2-x1+1, tile::empty))
        {}

        tile at(const point& pt) const {
            if (!contains(pt)) {
                return tile::empty;
            }
            return ary_[pt.y][pt.x - min_x_];
        }

        tile at(int x, int y) const {
            return at(point{ x,y });
        }

        bool contains(const point& pt) const {
            return pt.x >= min_x_ && pt.x < min_x_ + static_cast<int>(ary_[0].size()) &&
                pt.y >= 0 && pt.y < static_cast<int>(ary_.size());
        }

        void set(const point& pt, tile t) {
            ary_[pt.y][pt.x - min_x_] = t;
        }
    };

    auto points_between(const point& u, const point& v) {
        int from, to;
        if (u.x == v.x) {
            from = u.y;
            to = v.y;
        }
        if (u.y == v.y) {
            from = u.x;
            to = v.x;
        }
        if (from > to) {
            std::swap(from, to);
        }

        return rv::iota(from, to+1) |
            rv::transform(
                [u, v](int i)->point {
                    return (u.y == v.y) ? point{i, u.y} : point{u.x, i};
                }
            );
    }

    void draw_polyline(grid& g, const std::vector<point>& polyline) {
        for (auto line : polyline | rv::sliding(2)) {
            for (auto pt : points_between(line[0], line[1])) {
                g.set(pt, tile::wall);
            }
        }
    }

    std::vector<std::vector<point>> to_polylines(const std::vector<std::string>& lines) {
        return lines |
            rv::transform(
                [](const std::string& line)->std::vector<point> {
                    auto nums = aoc::extract_numbers(line);
                    return nums |
                        rv::chunk(2) |
                        rv::transform(
                            [](auto&& p)->point {
                                return { p[0], p[1] };
                            }
                        ) | r::to_vector;
                }
        ) | r::to_vector;
    }

    std::tuple<int, int, int, int> polylines_to_bounds(const std::vector<std::vector<point>>& polylines) {
        int x1 = 500;
        int y1 = 0;
        int x2 = 500;
        int y2 = 0;
        for (const auto& polyline : polylines) {
            for (const auto& pt : polyline) {
                x1 = (pt.x < x1) ? pt.x : x1;
                y1 = (pt.y < y1) ? pt.y : y1;
                x2 = (pt.x > x2) ? pt.x : x2;
                y2 = (pt.y > y2) ? pt.y : y2;
            }
        }
        return { x1,y1,x2,y2 };
     }

    bool drop_sand_grain(grid& g) {
        point pt = { 500,0 };
        if (g.at(pt) != tile::empty) {
            return false;
        }
        bool done = false;
        while (g.contains(pt)) {
            if (g.at(pt.x, pt.y + 1) == tile::empty) {
                pt = { pt.x, pt.y + 1 };
                continue;
            }
            if (g.at(pt.x - 1, pt.y + 1) == tile::empty) {
                pt = {pt.x - 1, pt.y + 1};
                continue;
            }
            if (g.at(pt.x + 1, pt.y + 1) == tile::empty) {
                pt = {pt.x + 1, pt.y + 1};
                continue;
            }
            break;
        } 
        if (g.contains(pt)) {
            g.set(pt, tile::sand);
            return true;
        }
        return false;
    }

    int drop_sand(grid& g) {
        bool added_sand;
        int count = 0;
        do {
            added_sand = drop_sand_grain(g);
            count += (added_sand) ? 1 : 0;
        } while (added_sand);

        return count;
    }

    int do_part_1(const std::vector<std::vector<point>>& polylines) {
        auto [x1, y1, x2, y2] = polylines_to_bounds(polylines);
        grid g(x1, x2, y2);
        for (const auto& polyline : polylines) {
            draw_polyline(g, polyline);
        }
        return drop_sand(g);
    }

    int do_part_2(const std::vector<std::vector<point>>& input) {
        auto polylines = input;
        auto [x1, y1, x2, y2] = polylines_to_bounds(polylines);
        y2 += 3;
        x1 = 500 - y2;
        x2 = 500 + y2;
        grid g(x1, x2, y2);
        polylines.push_back({ { x1, y2 - 1 }, { x2 - 1, y2 - 1 } } );
        for (const auto& polyline : polylines) {
            draw_polyline(g, polyline);
        }
        return drop_sand(g);
    }
}

/*------------------------------------------------------------------------------------------------*/

void aoc::day_14(const std::string& title) {
    auto input = file_to_string_vector(input_path(14, 1));
    auto polylines = to_polylines(input);

    std::cout << header(14, title);

    std::cout << "  part 1: " << do_part_1(polylines) << "\n";
    std::cout << "  part 2: " << do_part_2(polylines) << "\n";
}