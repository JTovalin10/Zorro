#pragma once

#include <filesystem>
#include <iostream>

#include "BuiltInCommand.hpp"

namespace fs = std::filesystem;

class PwdCommand : public BuiltInCommand {
 public:
  std::string Name() override { return "pwd"; }

  void Execute(const std::vector<std::string>& args) override {
    fs::path currentPath = fs::current_path();
    std::cout << currentPath.string() << "\n";
  }
};

static bool pwd_registry = []() {
  CommandRegistry::Add(std::unique_ptr<PwdCommand>());
  return true;
}();