#include <functional>
#include <iostream>
#include <unordered_map>

#include "../ShellHelper/History.hpp"
#include "BuiltInCommand.hpp"

class HistoryCommand : public BuiltInCommand {
 public:
  std::string Name() const override { return "history"; }

  void Execute(const std::vector<std::string>& args) override {
    if (args.size() == 1) {
      Slime::get_history().print();
      return;
    }

    const std::string& flag = args[1];
    auto it = handlers_.find(flag);
    if (it != handlers_.end()) {
      it->second(args);
    } else {
      // check if its an integer
      int num;
      try {
        num = std::stoi(flag);
      } catch (...) {
        std::cerr << "history: " << flag << ": invalid option\n";
      }
      Slime::get_history().print(num);
    }
  }

 private:
  using Handler = std::function<void(const std::vector<std::string>&)>;

  static const std::unordered_map<std::string, Handler> handlers_;
};

const std::unordered_map<std::string, HistoryCommand::Handler>
    HistoryCommand::handlers_ = {
        {"-c",
         [](const std::vector<std::string>& args) {
           if (args.size() != 2) {
             std::cerr << "history: -c: too many arguments\n";
             return;
           }
           Slime::get_history().clear();
         }},
        {"-d",
         [](const std::vector<std::string>& args) {
           if (args.size() != 3) {
             std::cerr << "history: -d: expected one argument\n";
             return;
           }
           int num = std::stoi(args[2]);
           if (num < 0) {
             std::cerr << "history: -d: invalid offset\n";
             return;
           }
           Slime::get_history().remove(num);
         }},
        {"-a",
         [](const std::vector<std::string>& args) {
           if (args.size() != 2) {
             std::cerr << "history: -a: too many arguments\n";
             return;
           }
           Slime::get_history().save();
         }},
        {"-r",
         [](const std::vector<std::string>& args) {
           if (args.size() != 3) {
             std::cerr << "history: -r: too few or many arguments\n";
             return;
           }
           const std::string file = args[2];
           Slime::get_history().read(file);
         }},
        {"-w",
         [](const std::vector<std::string>& args) {
           if (args.size() != 2) {
             std::cerr << "history: -w: too many arguments\n";
             return;
           }
           Slime::get_history().save();
         }},
};

static bool history_registered = []() {
  CommandRegistry::Add(std::make_unique<HistoryCommand>());
  return true;
}();
