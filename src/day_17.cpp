#include "util.h"
#include "days.h"
#include <range/v3/all.hpp>
#include <iostream>
#include <functional>
#include <array>
#include <boost/functional/hash.hpp>
#include <unordered_set>
#include <sstream>

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

    using shape_def = std::vector<point>;

    class shape {
        const shape_def def_;
        int wd_;
        int hgt_;
        shape_def left_;
        shape_def bottom_;
        shape_def right_;

        void populate_frontiers() {
            point_set pt_set = def_ | r::to<point_set>();
            for (auto&& pt : def_) {
                auto  left = pt - point{ 1, 0 };
                if (!pt_set.contains(left)) {
                    left_.push_back(left);
                }
                auto right = pt + point{ 1,0 };
                if (!pt_set.contains(right)) {
                    right_.push_back(right);
                }
                auto bottom = pt - point{ 0,1 };
                if (!pt_set.contains(bottom)) {
                    bottom_.push_back(bottom);
                }
            }
        }

    public:
        shape(const shape_def& def) :
            def_(def),
            wd_(r::max(def | rv::transform([](auto&& p) {return p.x;}))+1),
            hgt_(r::max(def | rv::transform([](auto&& p) {return p.y; })) + 1)
        {
            populate_frontiers();
        }

        auto operator()(const point& loc) const {
            return def_ | rv::transform(
                [&loc](auto& pt)->point {
                    return loc + pt;
                }
            );
        }

        auto left(const point& loc) const {
            return left_ | rv::transform(
                [&loc](auto& pt)->point {
                    return loc + pt;
                }
            );
        }

        auto right(const point& loc) const {
            return right_ | rv::transform(
                [&loc](auto& pt)->point {
                    return loc + pt;
                }
            );
        }

        auto bottom(const point& loc) const {
            return bottom_ | rv::transform(
                [&loc](auto& pt)->point {
                    return loc + pt;
                }
            );
        }

        int width() const {
            return wd_;
        }

        int height() const {
            return hgt_;
        }
    };

    class horz_move_stream {
        std::vector<int> impl_;
        int iter;
    public:
        horz_move_stream(const std::string& inp, int i = 0) :
            iter(i),
            impl_(inp | rv::transform([](char ch) {return ch == '<' ? -1 : 1; }) | r::to_vector)
        {}

        int operator++(int) {
            auto val = impl_[iter];
            iter = (iter + 1) % static_cast<int>(impl_.size());
            return val;
        }

        int state() const {
            return iter;
        }
    };

    class shape_stream {
        const std::vector<shape> shapes_;
        int iter;
    public:
        shape_stream(int i = 0) :
            shapes_( std::vector<shape> {
                shape({{0,0},{1,0},{2,0},{3,0}}),       // -
                shape({{0,1},{1,0},{1,1},{1,2},{2,1}}), // +
                shape({{0,0},{1,0},{2,0},{2,1},{2,2}}), // _|
                shape({{0,0},{0,1},{0,2},{0,3}}),       // |
                shape({{0,0},{1,0},{1,1},{0,1}})        // #
            }),
            iter(i)
        {}

        const shape& operator++(int) {
            const auto& val = shapes_[iter];
            iter = (iter + 1) % static_cast<int>(shapes_.size());
            return val;
        }

        const shape& operator*() {
            return shapes_[iter];
        }

        int state() const {
            return iter;
        }
    };

    std::vector<int> distances;

    class well {
        std::vector<uint8_t> impl_;

        bool drop_shape_one_unit(const shape& shape , horz_move_stream& horz_moves, point& loc) {
            // do horz motion
            auto horz = horz_moves++;
            if (horz < 0) {
                if (is_empty_set(shape.left(loc))) {
                    loc.x--;
                }
            } else {
                if (is_empty_set(shape.right(loc))) {
                    loc.x++;
                }
            }
            // do drop
            if (is_empty_set(shape.bottom(loc))) {
                loc.y--;
                return true;
            }
            return false;
        }

        void extend_height(int amt) {
            for (int i = 0; i < amt; ++i) {
                impl_.push_back(0);
            }
        }

        void lock_in_shape(const shape& shape, const point& loc) {
            int top = loc.y + shape.height();
            if (top > height()) {
                extend_height(top - height());
            }
            for (auto&& pt : shape(loc)) {
                impl_[pt.y] = impl_[pt.y] | (static_cast<uint8_t>(1) << pt.x);
            }
        }

    public:
        int height() const {
            return static_cast<int>(impl_.size());
        }

        uint8_t top() const {
            return impl_.back();
        }

        bool is_empty(const point& pt) const {
            if (pt.x < 0 || pt.x >= 7) {
                return false;
            }
            if (pt.y < 0) {
                return false;
            }
            if (pt.y >= height()) {
                return true;
            }
            return !(impl_[pt.y] & (static_cast<uint8_t>(1) << pt.x));
        }

        bool is_empty_set(auto pts) const {
            for (auto&& pt : pts) {
                if (!is_empty(pt)) {
                    return false;
                }
            }
            return true;
        }

        void drop_shape(shape_stream& shapes, horz_move_stream& horz_moves) {
            bool in_motion = true;
            point loc = { 2, height() + 3 };
            const auto& shape = shapes++;
            do {
                in_motion = drop_shape_one_unit(shape, horz_moves, loc);
            } while (in_motion);
            lock_in_shape( shape, loc);
        }

        std::string paint() const {
            auto rows = rv::reverse(impl_) |
                rv::transform(
                    [](uint8_t row_byte)->std::string {
                        std::string row = ".......";
                        for (int x = 0; x < 7; ++x) {
                            if (row_byte & (static_cast<uint8_t>(1) << x)) {
                                row[x] = '#';
                            }
                        }
                        return row;
                    }
            );
            std::stringstream ss;
            for (auto&& row : rows) {
                ss << row << "\n";
            }
            return ss.str();
        }
    };

    std::tuple<well, int, int> well_after_n_drops(const std::string& input, int n) {
        horz_move_stream horz_moves(input);
        shape_stream shapes;
        ::well well;
        for (int i = 0; i < n; ++i) {
            well.drop_shape(shapes, horz_moves);
        }
        return { std::move(well), shapes.state(), horz_moves.state() };
    }

    int height_after_n_drops(const std::string& input, int n) {
        auto [well, dummy1, dummy2] = well_after_n_drops(input, n);
        return well.height();
    }

    std::vector<int> make_cycle_table(const std::string& input, uint64_t drops_preamble, uint64_t drops_cycle) {
        auto [well, shape_iter, move_iter] = well_after_n_drops(input, static_cast<int>(drops_preamble));
        int base_height = well.height();
        std::vector<int> tbl(drops_cycle);
        horz_move_stream horz_moves(input, move_iter);
        shape_stream shapes(shape_iter);
        tbl[0] = 0;
        for (uint64_t i = 1; i < drops_cycle; ++i) {
            well.drop_shape(shapes, horz_moves);
            tbl[i] = well.height() - base_height;
        }
        return tbl;
    }

    uint64_t calculate_height_of_n_drops_using_cycles(const std::string& input, uint64_t n) {
        const uint64_t k_drops_preamble = 974;
        const uint64_t k_drops_cycle = 1695;
        const uint64_t k_hgt_preamble = 1519;
        const uint64_t k_hgt_cycle = 2634;

        uint64_t x = (n - k_drops_preamble) / k_drops_cycle;
        uint64_t slop = (n - k_drops_preamble) % k_drops_cycle;

        const static auto tbl = make_cycle_table(input, k_drops_preamble, k_drops_cycle);

        return k_hgt_preamble + x * k_hgt_cycle + tbl[slop];
    }
}

void aoc::day_17(const std::string& title) {
    auto input = file_to_string(input_path(17, 1));
    input = input | rv::take(input.size() - 1) | r::to<std::string>;
    std::cout << header(17, title);

    std::cout << "  part 1: " << height_after_n_drops(input, 2022) << "\n";
    std::cout << "  part 2: " << calculate_height_of_n_drops_using_cycles(input, 1000000000000) << "\n";

}

