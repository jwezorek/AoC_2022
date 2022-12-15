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

    namespace comp_geom_in_C {
        /*
            C code cut-and-pasted from O'Rourke "Computational Geometry in C", 2nd Edition.
        */

        #define	EXIT_FAILURE 1
        #define	X 0
        #define	Y 1
        #define	DIM 2               /* Dimension of points */
        typedef	int tPointi[DIM];   /* Type integer point */
        typedef	double tPointd[DIM];   /* Type double point */


        char ParallelInt(tPointi a, tPointi b, tPointi c, tPointi d, tPointd p);
        void Assigndi(tPointd p, tPointi a);
        bool Between(tPointi a, tPointi b, tPointi c);
        int Collinear(tPointi a, tPointi b, tPointi c);
        int AreaSign(tPointi a, tPointi b, tPointi c);

        /*---------------------------------------------------------------------
        SegSegInt: Finds the point of intersection p between two closed
        segments ab and cd.  Returns p and a char with the following meaning:
           'e': The segments collinearly overlap, sharing a point.
           'v': An endpoint (vertex) of one segment is on the other segment,
                but 'e' doesn't hold.
           '1': The segments intersect properly (i.e., they share a point and
                neither 'v' nor 'e' holds).
           '0': The segments do not intersect (i.e., they share no points).
        Note that two collinear segments that share just one point, an endpoint
        of each, returns 'e' rather than 'v' as one might expect.
        ---------------------------------------------------------------------*/
        char SegSegInt(tPointi a, tPointi b, tPointi c, tPointi d, tPointd p)
        {
            double  s, t;       /* The two parameters of the parametric eqns. */
            double num, denom;  /* Numerator and denoninator of equations. */
            char code = '?';    /* Return char characterizing intersection. */

            denom = a[X] * (double)(d[Y] - c[Y]) +
                b[X] * (double)(c[Y] - d[Y]) +
                d[X] * (double)(b[Y] - a[Y]) +
                c[X] * (double)(a[Y] - b[Y]);

            /* If denom is zero, then segments are parallel: handle separately. */
            if (denom == 0.0)
                return  ParallelInt(a, b, c, d, p);

            num = a[X] * (double)(d[Y] - c[Y]) +
                c[X] * (double)(a[Y] - d[Y]) +
                d[X] * (double)(c[Y] - a[Y]);
            if ((num == 0.0) || (num == denom)) code = 'v';
            s = num / denom;
            //printf("num=%lf, denom=%lf, s=%lf\n", num, denom, s);

            num = -(a[X] * (double)(c[Y] - b[Y]) +
                b[X] * (double)(a[Y] - c[Y]) +
                c[X] * (double)(b[Y] - a[Y]));
            if ((num == 0.0) || (num == denom)) code = 'v';
            t = num / denom;
            //printf("num=%lf, denom=%lf, t=%lf\n", num, denom, t);

            if ((0.0 < s) && (s < 1.0) &&
                (0.0 < t) && (t < 1.0))
                code = '1';
            else if ((0.0 > s) || (s > 1.0) ||
                (0.0 > t) || (t > 1.0))
                code = '0';

            p[X] = a[X] + s * (b[X] - a[X]);
            p[Y] = a[Y] + s * (b[Y] - a[Y]);

            return code;
        }

        char ParallelInt(tPointi a, tPointi b, tPointi c, tPointi d, tPointd p)
        {
            if (!Collinear(a, b, c))
                return '0';

            if (Between(a, b, c)) {
                Assigndi(p, c);
                return 'e';
            }
            if (Between(a, b, d)) {
                Assigndi(p, d);
                return 'e';
            }
            if (Between(c, d, a)) {
                Assigndi(p, a);
                return 'e';
            }
            if (Between(c, d, b)) {
                Assigndi(p, b);
                return 'e';
            }
            return '0';
        }

        void Assigndi(tPointd p, tPointi a)
        {
            int i;
            for (i = 0; i < DIM; i++)
                p[i] = a[i];
        }
        /*---------------------------------------------------------------------
        Returns TRUE iff point c lies on the closed segement ab.
        Assumes it is already known that abc are collinear.
        ---------------------------------------------------------------------*/
        bool  Between(tPointi a, tPointi b, tPointi c)
        {
            /* If ab not vertical, check betweenness on x; else on y. */
            if (a[X] != b[X])
                return ((a[X] <= c[X]) && (c[X] <= b[X])) ||
                ((a[X] >= c[X]) && (c[X] >= b[X]));
            else
                return ((a[Y] <= c[Y]) && (c[Y] <= b[Y])) ||
                ((a[Y] >= c[Y]) && (c[Y] >= b[Y]));
        }
        int Collinear(tPointi a, tPointi b, tPointi c)
        {
            return AreaSign(a, b, c) == 0;
        }

        int AreaSign(tPointi a, tPointi b, tPointi c)
        {
            double area2;

            area2 = (b[0] - a[0]) * (double)(c[1] - a[1]) -
                (c[0] - a[0]) * (double)(b[1] - a[1]);

            /* The area should be an integer. */
            if (area2 > 0.5) return  1;
            else if (area2 < -0.5) return -1;
            else                     return  0;
        }
    }

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
        int a1_ary[2] = { a1.x, a1.y };
        int a2_ary[2] = { a2.x, a2.y };
        int b1_ary[2] = { b1.x, b1.y };
        int b2_ary[2] = { b2.x, b2.y };
        double inter_ary[2] = {
            std::numeric_limits<double>::quiet_NaN(),
            std::numeric_limits<double>::quiet_NaN()
        };
        char code = comp_geom_in_C::SegSegInt(
            a1_ary, a2_ary, b1_ary, b2_ary, inter_ary
        );
        if (code == '0' || code == 'e') {
            return {};
        }
        auto is_integer = [](double v) {
            if (std::abs(v) - std::floor(std::abs(v)) > 0.000001) {
                return false;
            }
            return true;
        };

        if (!is_integer(inter_ary[0]) || !is_integer(inter_ary[1])) {
            return {};
        }

        return point{
            static_cast<int>(inter_ary[0]),
            static_cast<int>(inter_ary[1])
        };
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