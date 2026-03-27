#include <cstdlib>
#include <iostream>
#include <ranges>
#include <string>
#include <vector>

#include "FileSys/FileSys.hpp"
#include "ShellHelper/History.hpp"
#include "ShellHelper/ShellHelper.hpp"

/**
 * Helper function that completes all other operations that are not exit
 *
 * ARGS:
 * user_input - string that contains the user input
 */
void complete_operation(const std::string& user_input) noexcept {
  std::vector<std::string> inputs = Slime::parse_args(user_input);
  std::string command = inputs[0];
  if (Slime::is_built_in(command) || Slime::is_executable(command)) {
    Slime::execa(inputs);
  } else {
    std::cout << user_input << ": command not found\n";
  }
}

int main() {
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  rl_attempted_completion_function = Slime::autocomplete;
  Slime::insert_files_in_trie();
  std::string user_input{};

  Slime::History& hist = Slime::get_history();
  while (true) {
    char* input = readline("$ ");

    if (!input) continue;

    if (*input == '\0') {
      free(input);
      continue;
    }

    if (input) {
      user_input = input;
      free(input);
    }

    hist.add(user_input);
    if (user_input == "exit") break;

    complete_operation(user_input);
  }
  return 0;
}
