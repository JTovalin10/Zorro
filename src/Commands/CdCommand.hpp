#pragma once

#include <filesystem>
#include <iostream>

#include "BuiltInCommand.hpp"

namespace fs = std::filesystem;

class CdCommand : public BuiltInCommand {
 public:
  std::string Name() override { return "cd"; }

  void Execute(const std::vector<std::string>& args) override {
    std::string desired_path = args[1];
    if (fs::exists(desired_path) && fs::is_directory(desired_path)) {
      try {
        fs::current_path(desired_path);
      } catch (const fs::filesystem_error& e) {
        // asumes the path is correct
        std::cerr << "cd: " << desired_path << ": No such file or directory\n";
      }
    } else {
      std::cerr << "cd: " << desired_path << ": No such file or directory\n";
    }
  }
};  // namespace std::filesystem

static bool cd_registred = []() {
  CommandRegistry::Add(std::make_unique<CdCommand>());
  return true;
}();