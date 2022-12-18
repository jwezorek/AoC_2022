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

    const std::vector<shape> shapes = {
        shape({{0,0},{1,0},{2,0},{3,0}}),       // -
        shape({{0,1},{1,0},{1,1},{1,2},{2,1}}), // +
        shape({{0,0},{1,0},{2,0},{2,1},{2,2}}), // _|
        shape({{0,0},{0,1},{0,2},{0,3}}),       // |
        shape({{0,0},{1,0},{1,1},{0,1}})        // #
    };

    using horz_offsets = r::any_view<int>;

    class well {
        std::vector<uint8_t> impl_;

        bool drop_shape_one_unit(const shape& shape, point& loc, auto& horz_stream) {
            // do horz motion
            auto horz = *(horz_stream++);
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

        void drop_shape(const shape& shape, auto& horz_stream) {
            bool in_motion = true;
            point loc = { 2, height() + 3 };
            do {
                in_motion = drop_shape_one_unit(shape, loc, horz_stream);
            } while (in_motion);
            lock_in_shape(shape, loc);
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

}

void aoc::day_17(const std::string& title) {
    auto input = file_to_string(input_path(17, 1));
    auto horz_moves = input | rv::take(input.size()-1) |
        rv::transform([](char ch) {return ch == '<' ? -1 : 1; }) | r::to_vector;

    auto horz = horz_moves | r::to_vector;
    auto horz_stream = horz_moves | rv::cycle;
    auto horz_iter = horz_stream.begin();
    well w;
    for (int i = 0; i < 2022; ++i) {
        int shape_index = i % shapes.size();
        w.drop_shape(shapes[shape_index], horz_iter);
    }
    //std::cout << w.paint() << "\n\n";
    std::cout << header(17, title);
    
    std::cout << "  part 1: " << w.height() << "\n";
    std::cout << "  part 2: " << 0 << "\n";
}