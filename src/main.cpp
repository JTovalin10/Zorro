#include <iostream>
#include <string>


bool is_input_shell_type(const std::string& input) noexcept {
  switch (input) {
    case "echo":  return true;
    case "break": return true;
    default: return false;
  }
}

/**
 * Helper function that completes all other operations that are not exit
 *
 * ARGS:
 * user_input - string that contains the user input
 */
void complete_operation(const std::string& user_input) noexcept {
  // check if is echo
  const bool geq_4 = user_input.size() >= 4;
  if (geq_4 && user_input.substr(0, 4) == "echo") {
    std::cout << user_input.substr(5);
  } else if (geq_4 && user.substr(0, 4) == "type") {
    const bool is_type = is_input_shell_type(user.substr(5));
    if (is_type) {
      std::cout << user_input.substr(0, 4) << " is a shell builtin";
    } else {
      std::cout << user_input << ": not found";
    }
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

