#include "util.h"
#include "days.h"
#include <range/v3/all.hpp>
#include <vector>
#include <string>
#include <unordered_map>
#include <iostream>
#include <memory>
#include <functional>
#include <stdexcept>

namespace r = ranges;
namespace rv = ranges::views;

/*------------------------------------------------------------------------------------------------*/

namespace {

    struct directory {
        std::unordered_map<std::string, int> files;
        std::unordered_map<std::string, std::shared_ptr<directory>> directories;
        std::weak_ptr<directory> parent;
        int size;

        directory() : size(0)
        {}
    };

    using dir_ptr = std::shared_ptr<directory>;
    using command = std::function<dir_ptr(const dir_ptr& current_dir)>;
    using command_parser = std::function<command(const std::string& line)>;

    command parse_create_root_dir(const std::string& line) {
        if (line == "$ cd /") {
            return [](const dir_ptr& current_dir)->dir_ptr {
                return std::make_shared<directory>();
            };
        }
        return {};
    }
 
    command parse_ls_command(const std::string& line) {
        if (line == "$ ls") {
            return [](const dir_ptr& current_dir)->dir_ptr {
                // noop ... we'll just treat the listed files
                // and directories as separate commands
                return {};
            };
        } 
        return {};
    }

    command parse_create_directory(const std::string& line) {
        auto pieces = aoc::split(line, ' ');
        if (pieces.size() == 2 && pieces[0] == "dir") {
            return [dir_name = pieces[1]](const dir_ptr& current_dir)->dir_ptr {
                auto new_directory = std::make_shared<directory>();
                new_directory->parent = std::weak_ptr<directory>(current_dir);
                current_dir->directories[dir_name] = new_directory;
                return {};
            };
        } 
        return {};
    }

    command parse_create_file(const std::string& line) {
        auto pieces = aoc::split(line, ' ');
        if (pieces.size() == 2 && aoc::is_number(pieces[0])) {
            int sz = std::stoi(pieces[0]);
            return [fname = pieces[1], sz](const dir_ptr& current_dir)->dir_ptr {
                current_dir->files[fname] = sz;
                return {};
            };
        }
        return {};
    }
        
    command parse_cd_to_parent(const std::string& line) {
        if (line == "$ cd ..") {
            return [](const dir_ptr& current_dir)->dir_ptr {
                return current_dir->parent.lock();
            };
        } 
        return {};
    }
    
    command parse_cd_to_child(const std::string& line) {
        auto pieces = aoc::split(line, ' ');
        if (pieces.size() == 3 && pieces[0] == "$" && 
                pieces[1] == "cd" && pieces[2] != "..") {
            return [dir_name = pieces[2]](const dir_ptr& current_dir)->dir_ptr {
                return current_dir->directories[dir_name];
            };
        }
        return {};
    }

    command parse_line(const std::string& line) {
        const static std::vector<command_parser> parsers = {
            parse_create_root_dir,
            parse_ls_command,
            parse_create_directory,
            parse_create_file,
            parse_cd_to_parent,
            parse_cd_to_child
        };
        for (const auto& parser : parsers) {
            auto cmd = parser(line);
            if (cmd) {
                return cmd;
            }
        }
        throw std::runtime_error("unknown command: " + line);
    }

    auto child_directories(dir_ptr dir) {
        return dir->directories | rv::transform([](auto pair) {return pair.second; });
    }

    void populate_directory_sizes(dir_ptr root) {
        for (auto child : child_directories(root)) {
            populate_directory_sizes(child);
        }
        auto size_of_files = r::accumulate(
            root->files | rv::transform([](auto pair) {return pair.second; }), 
            0
        );
        auto size_of_directories = r::accumulate(
            child_directories(root) | rv::transform([](auto child) {return child->size; }),
            0
        );
        root->size = size_of_files + size_of_directories;
    }

    dir_ptr construct_file_system(auto commands) {
        dir_ptr root;
        dir_ptr current_dir;
        for (auto cmd : commands) {
            auto new_dir = cmd(current_dir);
            if (!root) {
                root = new_dir;
            }
            if (new_dir) {
                current_dir = new_dir;
            }
        }
        populate_directory_sizes(root);
        return root;
    }

    int size_of_directories_at_most_100000(dir_ptr root) {
        int size = (root->size <= 100000) ? root->size : 0;
        for (auto child : child_directories(root)) {
            size += size_of_directories_at_most_100000(child);
        }
        return size;
    }

    constexpr auto total_disk_space = 70000000;
    constexpr auto space_required = 30000000;

    int size_of_smallest_file_to_delete(dir_ptr root) {
        auto unused_space = total_disk_space - root->size;
        auto space_needed_to_free_up = space_required - unused_space;
        std::vector<int> large_enough_dir_sizes;

        std::function<void(dir_ptr)> find_large_enough_dirs;
        find_large_enough_dirs = [&](dir_ptr p) {
            for (auto child : child_directories(p)) {
                if (child->size > space_needed_to_free_up) {
                    large_enough_dir_sizes.push_back(child->size);
                    find_large_enough_dirs(child);
                }
            }
        };

        find_large_enough_dirs(root);
        return r::min(large_enough_dir_sizes);
    }
}

void aoc::day_7(int day, const std::string& title) {
    auto input = file_to_string_vector(input_path(7, 1));
    auto commands = input |
        rv::transform([](const std::string& line) {return parse_line(line); });

    auto root_dir = construct_file_system(commands);
    
    std::cout << header(day, title);
    std::cout << "   part 1: " << size_of_directories_at_most_100000(root_dir) << "\n";
    std::cout << "   part 2: " << size_of_smallest_file_to_delete(root_dir) << "\n";
}