#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class BuiltInCommand {
 public:
  virtual ~BuiltInCommand() = default;
  virtual std::string Name() = 0;
  virtual void Execute(const std::vector<std::string>& args) = 0;
};

using CommandFunc = std::function<void(const std::vector<std::string>&)>;

class CommandRegistry {
 public:
  static void Add(std::unique_ptr<BuiltInCommand> cmd) {
    GetMap()[cmd->Name()] = std::move(cmd);
  }

  static bool Run(const std::string& name,
                  const std::vector<std::string>& args) {
    auto& map = GetMap();
    if (map.find(name) != map.end()) {
      map[name]->Execute(args);
      return true;
    }
    return false;
  }

  static bool IsBuiltIn(const std::string& name) {
    auto& map = GetMap();
    return map.find(name) != map.end();
  }

 private:
  static std::unordered_map<std::string, std::unique_ptr<BuiltInCommand>>&
  GetMap() {
    static std::unordered_map<std::string, std::unique_ptr<BuiltInCommand>>
        instance;
    return instance;
  }
};