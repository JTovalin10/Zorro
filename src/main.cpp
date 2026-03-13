#include "filesys.h"
#include "shell_built_in.h"
#include <cstdlib>
#include <iostream>
#include <string>

bool is_input_shell_type(const std::string &input) noexcept {
  static shell_hash_set set{};
  return set.contains(input);
}

const std::string find_in_file_system(std::string &command) noexcept {
  char *path = std::getenv("PATH");
  // base case
  return Slime::find_in_path(command, path);
}

void print_type(const std::string &input) noexcept {
  std::string type = input.substr(5);
  const bool is_type = is_input_shell_type(type);
  if (is_type) {
    std::cout << type << " is a shell builtin";
    return;
  }
  // check if it is within our file system
  const std::string &path = find_in_file_system(type);

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
void complete_operation(const std::string &user_input) noexcept {
  // check if is echo
  const bool geq_4 = user_input.size() >= 4;
  if (geq_4 && user_input.substr(0, 4) == "echo") {
    std::cout << user_input.substr(5);
  } else if (geq_4 && user_input.substr(0, 4) == "type") {
    print_type(user_input);
  } else {
    std::cout << user_input << ": command not found";
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
    std::cout << std::endl;
    user_input.clear();
  }
}
