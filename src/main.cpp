#include <cstdlib>
#include <iostream>
#include <string>

#include "filesys.h"

void print_type(const std::string& input) noexcept {
  std::string type = input.substr(5);
  const bool is_type = Slime::is_input_shell_type(type);
  if (is_type) {
    std::cout << type << " is a shell builtin";
    return;
  }
  // check if it is within our file system
  const std::string& path = Slime::find_in_file_system(type);

  if (path.size() > 0) {
    std::cout << type << " is " << path;
    return;
  }
  // base case
  std::cout << type << ": not found";
}

/**
 * Helper function that completes all other operations that are not exit
 *
 * ARGS:
 * user_input - string that contains the user input
 */
void complete_operation(const std::string& user_input) noexcept {
  std::vector<std::string> inputs = user_input | std::views::split(' ') |
                                    std::ranges::to<std::vector<std::string>>();
  std::string command = inputs[0];
  if (command == "echo") {
    // length of command + blank space
    std::cout << user_input.substr(command.length() + 1) << "\n";
  } else if (command == "type") {
    print_type(user_input);
    std::cout << "\n";
  } else if (command == "pwd") {
    Slime::print_working_directory();
  } else if (Slime::is_executable(command, std::getenv("PATH"))) {
    Slime::execute_command(command, inputs);
  } else {
    std::cout << user_input << ": command not found\n";
  }
}

int main() {
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;
  std::string user_input;
  while (true) {
    std::cout << "$ ";
    // use get line to get the entire line
    std::getline(std::cin, user_input);
    if (user_input == "exit") {
      break;
    }
    complete_operation(user_input);
    user_input.clear();
  }
}
