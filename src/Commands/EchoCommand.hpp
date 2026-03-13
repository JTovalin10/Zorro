#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "BuiltInCommand.hpp"

class EchoCommand : public BuiltInCommand {
 public:
  std::string Name() override { return "echo"; }

  void Execute(const std::vector<std::string>& args) override {
    std::cout << args.at(0).size() << "\n";
  }
};

static bool echo_registered = []() {
  CommandRegistry::Add(std::make_unique<EchoCommand>());
  return true;
}();