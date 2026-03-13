#ifndef FILESYS_H_
#define FILESYS_H_

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

#include "shell_built_in.h"

namespace fs = std::filesystem;

namespace Slime {

void change_directory(const std::string& desired_path) {
  fs::path current_path = fs::current_path();
  try {
    fs::current_path(desired_path);
  } catch (const fs::filesystem_error& e) {
    std::cout << "cd: " << desired_path << ": No such file or directory";
  }
}

void print_working_directory() {
  fs::path currentPath = fs::current_path();
  std::cout << currentPath.string() << "\n";
}

void execute_command(const std::string& command,
                     std::vector<std::string> inputs) {
  // executes the command
  std::string full_argument = command;
  for (int i = 1; i < inputs.size(); ++i) {
    full_argument += " " + inputs[i];
  }
  int system = std::system(full_argument.c_str());
}

std::vector<std::string> get_directories(const char* char_path) {
  std::string_view path(char_path);
  std::vector<std::string> result;

  for (auto&& word : path | std::views::split(':')) {
    result.emplace_back(word.begin(), word.end());
  }
  return result;
}

bool check_file_permission_status(const fs::path& path) {
  auto status = fs::status(path);
  auto permissions = status.permissions();
  // if ((permissions & fs::perms::owner_write) != fs::perms::none) return
  // false; if ((permissions & fs::perms::owner_read) != fs::perms::none)
  // return false;
  if ((permissions & fs::perms::owner_exec) != fs::perms::none) return true;
  return false;
}

std::string find_in_path(const std::string& command, const char* path) {
  if (!path) {
    return "";
  }
  std::vector<std::string> directories = std::move(get_directories(path));

  for (const auto& dir : directories) {
    fs::path full_path = fs::path(dir) / command;
    if (fs::exists(full_path)) {
      if (check_file_permission_status(full_path) == false) continue;
      return full_path.string();
    }
  }
  return "";
}

bool is_executable(const std::string& command, const char* path) {
  std::string&& full_path = std::move(find_in_path(command, path));
  return full_path.size() > 0;
}

bool is_input_shell_type(const std::string& input) noexcept {
  static shell_hash_set set{};
  return set.contains(input);
}

const std::string find_in_file_system(std::string& command) noexcept {
  char* path = std::getenv("PATH");
  // base case
  return find_in_path(command, path);
}

}  // namespace Slime

#endif  // FILESYS_H_
