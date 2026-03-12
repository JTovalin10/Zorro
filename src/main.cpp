#include <iostream>
#include <string>

int main() {
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  std::cout << "$ ";
  std::string user_input;
  std::cin >> user_input;
  if (user_input.size() >= 0) {
    std::cout << user_input + ": Command not found";
  }
}
