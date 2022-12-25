#include "util.h"
#include "days.h"
#include <range/v3/all.hpp>
#include <iostream>
#include <functional>
#include <sstream>
#include <deque>

namespace r = ranges;
namespace rv = ranges::views;

/*------------------------------------------------------------------------------------------------*/

namespace {

    using number = uint64_t;
    using digits = std::vector<int8_t>;

    number log_base_5(number n) {
        return static_cast<number>(
            std::log(static_cast<double>(n)) / std::log(5.0)
        );
    }

    number to_the_5th_power(number n) {
        return static_cast<number>(std::pow(5, n));
    }

    digits to_base_5(number num) {
        auto power_of_5 = to_the_5th_power(log_base_5(num));
        auto value = num;
        std::deque<int8_t> base_5_digits;
        while (power_of_5 > 1) {
            base_5_digits.push_front(
                static_cast<int8_t>(value / power_of_5)
            );
            value = value % power_of_5;
            power_of_5 /= 5;
        } 
        base_5_digits.push_front(value);

        return base_5_digits | r::to_vector;
    }

    void increment_base_5_digit(digits& ds, int i) {
        if (i < ds.size()) {
            ds[i]++;
        } else {
            ds.push_back(1);
        }
        if (ds[i] >= 5) {
            ds[i] = ds[i] % 5;
            increment_base_5_digit(ds, i + 1);
        }
    }

    digits to_snafu_number_digits(number num) {
        auto snafu_digits = to_base_5(num);
        int n = static_cast<int>(snafu_digits.size());
        for (int i = 0; i < n; ++i) {
            auto digit = snafu_digits[i];
            if (digit <= 2) {
                continue;
            }
            if (digit == 3) {
                snafu_digits[i] = -2;
            } else if (digit == 4) {
                snafu_digits[i] = -1;
            } 

            increment_base_5_digit(snafu_digits, i + 1);
        }
        return snafu_digits;
    }

    std::string to_string(const digits& digits) {
        return rv::reverse(digits) |
            rv::transform(
                [](int8_t digit)->char {
                    if (digit >= 0) {
                        return '0' + digit;
                    }
                    if (digit == -1) {
                        return '-';
                    }
                    if (digit == -2) {
                        return '=';
                    }
                }
        ) | r::to<std::string>();
    }

    number snafu_numeral_to_digit(char numeral) {
        if (numeral >= '0' && numeral <= '2') {
            return static_cast<number>(numeral - '0');
        }
        if (numeral == '-') {
            return -1;
        } 
        if (numeral == '=') {
            return -2;
        }
        throw std::runtime_error("bad snafu digit");
    }

    number from_snafu_number(const std::string& snafu) {
        number fives = 1;
        number value = 0;
        for (char numeral : rv::reverse(snafu)) {
            value += snafu_numeral_to_digit(numeral) * fives;
            fives *= static_cast<number>(5);
        }
        return value;
    }

    number sum_of_snafu_numbers(const std::vector<std::string>& nums) {
        return r::accumulate(
            nums | rv::transform(from_snafu_number),
            static_cast<number>(0)
        );
    }

    std::string to_snafu_number(number num) {
        return to_string(
            to_snafu_number_digits(num)
        );
    }
}

void aoc::day_25(const std::string& title) {

    auto input = file_to_string_vector(input_path(25, 1));

    std::cout << header(25, title);
    std::cout << "  part 1: " << to_snafu_number(sum_of_snafu_numbers(input)) << "\n";
    std::cout << "  part 2: " << 0 << "\n";
}