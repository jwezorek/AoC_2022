#include "util.h"
#include "days.h"
#include <range/v3/all.hpp>
#include <boost/functional/hash.hpp>
#include <iostream>
#include <functional>
#include <unordered_set>
#include <unordered_map>
#include <sstream>
#include <array>
#include <optional>
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

    struct point_hasher {
        size_t operator()(const point& pt) const {
            size_t seed = 0;
            boost::hash_combine(seed, pt.x);
            boost::hash_combine(seed, pt.y);
            return seed;
        }
    };

    using point_set = std::unordered_set<point, point_hasher>;
    
    template<typename T>
    using point_map = std::unordered_map<point, T, point_hasher>;

    struct circle {
        point center;
        int radius;
    };

    auto vertices(const circle& c) {
        static const std::array<point, 4> vertices = { { {0,-1}, {1,0}, {0,1}, {-1,0} } };
        return vertices |
            rv::transform(
                [&c](auto&& v)->point {
                    return c.center + (c.radius * v);
                }
        );
    }

    using line_segment = std::tuple<point, point>;

    auto edges(const circle& c) {
        return vertices(c) |
            rv::cycle |
            rv::sliding(2) |
            rv::take(4) |
            rv::transform(
                [](auto&& rng)->line_segment {
                    return { rng[0],rng[1] };
                }
            );
    }

    std::optional<point> line_segment_intersection(const line_segment& a, const line_segment& b) {
        const auto& [a1, a2] = a;
        const auto& [b1, b2] = b;
        int64_t x1 = a1.x;
        int64_t y1 = a1.y;
        int64_t x2 = a2.x;
        int64_t y2 = a2.y;
        int64_t x3 = b1.x;
        int64_t y3 = b1.y;
        int64_t x4 = b2.x;
        int64_t y4 = b2.y;

        int64_t denom = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);

        if (denom == 0) {
            return {};
        }

        int64_t t_numer = (x1 - x3) * (y3 - y4) - (y1 - y3) * (x3 - x4);
        int64_t u_numer = (x1 - x3) * (y1 - y2) - (y1 - y3) * (x1 - x2);
        int64_t x_diff_scaled = t_numer * (x2 - x1);
        int64_t y_diff_scaled = t_numer * (y2 - y1);

        if (x_diff_scaled % denom != 0 || y_diff_scaled % denom != 0) {
            return {};
        }

        // the if either numerator is greater than the denominator then
        // the intersection is not within one of the line segments.
        if (std::abs(t_numer) > std::abs(denom) || std::abs(u_numer) > std::abs(denom)) {
            return {};
        }

        auto intersection = point{
            static_cast<int>(x1 + x_diff_scaled / denom),
            static_cast<int>(y1 + x_diff_scaled / denom)
        };

        return intersection;
    };

    std::vector<point> circle_intersection(const circle& c_a, const circle& c_b) {
        auto edges_a = edges(c_a) | r::to_vector;
        auto edges_b = edges(c_b) | r::to_vector;

        std::vector<point> intersections;
        for (const auto [a, b] : rv::cartesian_product(rv::all(edges_a), rv::all(edges_b))){
            auto intersection = line_segment_intersection(a, b);
            if (intersection) {
                intersections.push_back(*intersection);
            }
        }

        return intersections;
    }

    struct rect {
        int x1;
        int y1;
        int x2;
        int y2;
    };

    bool pt_in_rect(const rect& r, const point& pt) {
        return pt.x >= r.x1 && pt.x <= r.x2 &&
            pt.y >= r.y1 && pt.y <= r.y2;
    }

    rect bounds_of_circle(const circle& c) {
        return {
            c.center.x - c.radius,
            c.center.y - c.radius,
            c.center.x + c.radius,
            c.center.y + c.radius
        };
    }

    rect union_rects(const rect& r1, const rect& r2) {
        return {
            std::min(r1.x1, r2.x1),
            std::min(r1.y1, r2.y1),
            std::max(r1.x2, r2.x2),
            std::max(r1.y2, r2.y2)
        };
    }

    rect bounds_of_circles(const std::vector<circle>& circles) {
        auto r = bounds_of_circle(circles.front());
        for (const auto& circ : circles) {
            auto circ_bounds = bounds_of_circle(circ);
            r = union_rects(r, circ_bounds);
        }
        return r;
    }

    int manhattan_distance(const point& u, const point& v) {
        auto diff = u - v;
        return std::abs(diff.x) + std::abs(diff.y);
    }

    std::tuple<std::vector<point>, std::vector<circle>> parse_beacons_and_sensors(const auto& inp) {
        auto tups = inp |
            rv::transform(
                [](const std::string& line)->std::tuple<point, circle> {
                    auto nums = aoc::extract_numbers(line);
                    point sensor = { nums[0], nums[1] };
                    point beacon = { nums[2], nums[3] };
                    return { beacon, circle{sensor, manhattan_distance(sensor,beacon)} };
                }
            ) | r::to_vector;

        auto beacon_set = tups |
            rv::transform([](auto&& tup) {return std::get<0>(tup); }) | r::to<point_set>();

        return {
            beacon_set | r::to_vector,
            tups | rv::transform([](auto&& tup) {return std::get<1>(tup); }) | r::to_vector
        };
    }

    bool pt_in_circle(const point& pt, const circle& c) {
        return manhattan_distance(pt, c.center) <= c.radius;
    }

    bool in_sensor_zone(const std::vector<circle>& sensors, const point& pt) {
        for (const auto& sensor : sensors) {
            if (pt_in_circle(pt, sensor)) {
                return true;
            }
        }
        return false;
    }

    int count_sensor_zones_in_row(const std::vector<circle>& sensors, const std::vector<point>& beacons, int row) {
        auto bounds = bounds_of_circles(sensors);
        if (row < bounds.y1 || row > bounds.y2) {
            return 0;
        }

        int num_beacons_on_row = r::accumulate(
            beacons | 
                rv::remove_if([row](auto&& b) {return b.y != row; }) | 
                rv::transform([](auto&&) {return 1; }),
            0
        );

        int count = 0;
        for (int x = bounds.x1; x <= bounds.x2; ++x) {
            point pt = { x, row };
            count += in_sensor_zone(sensors, pt) ? 1 : 0;
        }
        return count - num_beacons_on_row;
    }

    circle inflate(const circle& c) {
        return { c.center, c.radius + 1 };
    }

    auto corners(const rect& r) {
        return rv::concat(
            rv::single(point{ r.x1, r.y1 }),
            rv::single(point{ r.x2, r.y1 }),
            rv::single(point{ r.x2, r.y2 }),
            rv::single(point{ r.x1, r.y2 })
        );
    }

    uint64_t find_distress_beacon_tuning_freq(const std::vector<circle>& sensors, const std::vector<point>& beacons, const rect& bounds) {
        point_set critical_points;
        for (auto&& [c1, c2] : rv::cartesian_product(sensors, sensors)) {
            auto intersections = circle_intersection(inflate(c1), inflate(c2));
            for (auto pt : intersections) {
                critical_points.insert(pt);
            }
        }
        // add the corners of the bounds rect
        for (auto&& pt : corners(bounds)) {
            critical_points.insert(pt);
        }

        auto distress_beacon = critical_points |
            rv::remove_if(
                [&](const point& pt)->bool {
                    if (!pt_in_rect(bounds, pt)) {
                        return true;
                    }
                    if (in_sensor_zone(sensors, pt)) {
                        return true;
                    }
                    return false;
                }
            ) | r::to_vector;

        if (distress_beacon.size() == 1) {
            auto p = distress_beacon.front();
            return static_cast<uint64_t>(4000000) * static_cast<uint64_t>(p.x) + static_cast<uint64_t>(p.y);
        }
        return 0;
    }
}

void aoc::day_15(const std::string& title) {

    auto input = file_to_string_vector(input_path(15, 1));
    auto [beacons, sensors] = parse_beacons_and_sensors(input);

    std::cout << header(15, title);
    std::cout << "  part 1: " << count_sensor_zones_in_row(sensors, beacons, 2000000) << "\n";
    std::cout << "  part 2: " << find_distress_beacon_tuning_freq(sensors, beacons, { 0,0,4000000,4000000 }) << "\n";

}