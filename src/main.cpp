#include <iostream>
#include <string>

void complete_operation(const std::string& user_input) noexcept {
  // check if is echo
  std::cout << "size of string: " + user_input.size() << std::endl;
  if (user_input.size() >= 4) {
    std::cout << "Inside echo";
  } else {
    std::cout << user_input + ": command not found";
  }
}

int main() {
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;
  std::string user_input;
  while (true) {
    std::cout << "$ ";
    std::cin >> user_input;
    if (user_input == "exit") {
      break;
    }
    complete_operation(user_input);
    std::cout << std::endl;
    user_input.clear();
  }
}

