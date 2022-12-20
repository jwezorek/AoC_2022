#include "util.h"
#include "days.h"
#include <range/v3/all.hpp>
#include <iostream>
#include <functional>
#include <boost/intrusive/circular_list_algorithms.hpp>
#include <array>

namespace r = ranges;
namespace rv = ranges::views;

/*------------------------------------------------------------------------------------------------*/

namespace {

    struct circular_list_node {
        circular_list_node* next_;
        circular_list_node* prev_;
        int64_t value;
    };

    struct circular_list_node_traits
    {
        typedef circular_list_node node;
        typedef circular_list_node* node_ptr;
        typedef const circular_list_node* const_node_ptr;
        static node_ptr get_next(const_node_ptr n) { return n->next_; }
        static void set_next(node_ptr n, node_ptr next) { n->next_ = next; }
        static node* get_previous(const_node_ptr n) { return n->prev_; }
        static void set_previous(node_ptr n, node_ptr prev) { n->prev_ = prev; }
    };

    using circular = boost::intrusive::circular_list_algorithms<circular_list_node_traits>;

    class mix_list {
        std::vector<circular_list_node> nodes_;

        static circular_list_node num_to_node(int64_t v) {
            return { nullptr,nullptr,v };
        }

    public:
        mix_list(const std::vector<int64_t>& numbers) : 
            nodes_( numbers | rv::transform(num_to_node) | r::to_vector)
        {
            circular::init_header(&nodes_.front());
            for (auto pair : rv::iota(0, length()) | rv::sliding(2)) {
                circular::link_after(&nodes_[pair[0]], &nodes_[pair[1]]);
            }
        }

        int length() const {
            return static_cast<int>(nodes_.size());
        }

        void mix(int i) {
            auto n = length();
            auto distance = nodes_[i].value % static_cast<int64_t>(n-1);
            if (distance > 0) {
                circular::move_backwards(&nodes_[i], distance);
            } else if (distance < 0) {
                circular::move_forward(&nodes_[i], std::abs(distance));
            }
        }

        auto values() const {
            auto zero_item = r::find_if(
                nodes_, [](auto&& node) {return node.value == 0; }
            );
            struct generator {
                const circular_list_node* ptr;
                int64_t operator()() {
                    auto val = ptr->value;
                    ptr = ptr->next_;
                    return val;
                }
            };
            return rv::generate(generator{ &(*zero_item) }) | rv::take(length());
        }
    };

    int64_t mix(const std::vector<int64_t>& numbers, int n) {

        mix_list list(numbers);
        for (int j = 0; j < n; ++j) {
            for (int i = 0; i < list.length(); ++i) {
                list.mix(i);
            }
        }

        auto vec = list.values() | r::to_vector;
        std::array<int64_t, 3> indices = { {1000,2000,3000} };
        return r::accumulate(
            indices | rv::transform( 
                [&vec](int64_t i)->int64_t{
                    return vec[i % vec.size()]; 
                
                } 
            ),
            static_cast<int64_t>(0)
        );
    }
}

void aoc::day_20(const std::string& title) {
    auto input = file_to_string_vector(input_path(20, 1));

    auto part1_input = input | 
        rv::transform(
            [](auto&& str)->int64_t {return std::stoi(str); }
        ) | r::to_vector;

    auto part2_input = part1_input |
        rv::transform(
            [](auto v)->int64_t { return v * 811589153; }
        ) | r::to_vector;

    std::cout << header(20, title);
    std::cout << "  part 1: " << mix(part1_input, 1) << "\n";
    std::cout << "  part 2: " << mix(part2_input, 10) << "\n";
}