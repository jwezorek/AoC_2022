#include "days.h"
#include <string>
#include <sstream>
#include <filesystem>

/*------------------------------------------------------------------------------------------------*/

namespace fs = std::filesystem;

namespace {
    const std::string input_dir = "input";
    const std::string file_prefix = "day";
}

std::string aoc::input_path(int day, std::optional<int> part) {
    std::string part_tag = (part) ? std::to_string(*part) : std::string("test");
    std::string fname = file_prefix + "_" +
        std::to_string(day) + "_" +
        part_tag + ".txt";
    fs::path inp_path = fs::current_path() / input_dir / fname;
    return inp_path.string();
}

constexpr auto k_columns = 60;

std::string aoc::header(int day, const std::string& title) {
    auto label = std::string("AoC 2022 -- Day ") + std::to_string(day) + ": " + title;
    int num_stars = (k_columns - static_cast<int>(label.size()))/2;
    auto left_marg = std::string(num_stars-1, ' ');
    auto right_marg = std::string(k_columns - num_stars - label.size() - 1, ' ');
    std::stringstream ss;
    ss << std::string(k_columns, '*') << "\n";
    ss << '*' << left_marg << label << right_marg << "*\n";
    ss << std::string(k_columns, '*') << "\n\n";
    return ss.str();
}