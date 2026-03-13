#pragma once

#include <iostream>
#include <string>
#include <vector>

#include "../filesys.hpp"
#include "BuiltInCommand.hpp"

class TypeCommand : public BuiltInCommand {
 public:
  std::string Name() override { return "pwd"; }

  void Execute(const std::vector<std::string>& args) override {
    std::string type = args[0];
    const bool is_type = Slime::is_input_shell_type(type);
    if (is_type) {
      std::cout << type << " is a shell builtin\n";
      return;
    }
    // check if it is within our file system
    const std::string& path = Slime::find_in_file_system(type);

    if (path.size() > 0) {
      std::cout << type << " is " << path << "\n";
      return;
    }
    // base case
    std::cout << type << ": not found\n";
  }
};

static bool type_registred = []() {
  CommandRegistry::Add(std::unique_ptr<TypeCommand>());
  return true;
};