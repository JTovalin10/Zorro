#include "ShellHelper.hpp"

#include <algorithm>
#include <cstddef>

#include "Commands/BuiltInCommand.hpp"

enum class STATE { NORMAL, SINGLE_QUOTE, DOUBLE_QUOTE, BACKSLASH };

namespace SLIME_UTILS {
static constexpr char SQUOTE = '\'';
static constexpr char DQUOTE = '"';
static constexpr char BSLASH = '\\';
static constexpr char SPCHAR = ' ';

static constexpr int STDOUT_IDX{0};
static constexpr int STDOUT_APPEND_IDX{1};
static constexpr int STDERR_IDX{2};
static constexpr int STDERR_APPEND_IDX{3};
}  // namespace SLIME_UTILS

namespace Slime {
std::vector<std::string> parse_args(const std::string& user_args) {
  STATE state = STATE::NORMAL;
  std::vector<std::string> parsed_args{};
  std::string scurr{};
  const int size = user_args.size();
  parsed_args.reserve(size);

  for (int i = 0; i < size; ++i) {
    char curr = user_args[i];
    switch (state) {
      case STATE::NORMAL:
        if (curr == SLIME_UTILS::SQUOTE) {
          state = STATE::SINGLE_QUOTE;
        } else if (curr == SLIME_UTILS::DQUOTE) {
          state = STATE::DOUBLE_QUOTE;
        } else if (curr == SLIME_UTILS::BSLASH) {
          state = STATE::BACKSLASH;
        } else if (curr == SLIME_UTILS::SPCHAR) {
          if (!scurr.empty()) {
            parsed_args.push_back(scurr);
            scurr.clear();
          }
        } else {
          scurr += curr;
        }
        break;
      case STATE::SINGLE_QUOTE:
        if (curr == SLIME_UTILS::SQUOTE) {
          state = STATE::NORMAL;
        } else {
          scurr += curr;
        }
        break;
      case STATE::DOUBLE_QUOTE:
        if (curr == SLIME_UTILS::DQUOTE) {
          state = STATE::NORMAL;
        } else if (curr == SLIME_UTILS::BSLASH && i + 1 < size) {
          char next = user_args[i + 1];
          if (next == SLIME_UTILS::DQUOTE || next == SLIME_UTILS::BSLASH) {
            scurr += next;
            ++i;
          } else {
            scurr += curr;
          }
        } else {
          scurr += curr;
        }
        break;
      case STATE::BACKSLASH:
        // add the word into it and reset back to normal
        scurr += curr;
        state = STATE::NORMAL;
        break;
      default:
        break;
    }
  }
  if (!scurr.empty()) parsed_args.push_back(scurr);
  return parsed_args;
}

/**
 * Find the last directory as that is what we will output to.
 * anything that comes after the file will be treated as flags
 *
 * example: ls > example.txt hello -> ls hello > example.txt
 */
RedirectInfo find_redirect(std::vector<std::string>& args) {
  RedirectInfo info;
  std::vector<int> to_remove(4, -1);
  for (size_t i{}; i < args.size(); ++i) {
    const bool inbound = i + 1 < args.size();
    if ((args[i] == ">" || args[i] == "1>") && inbound) {
      info.stdout_file = args[i + 1];
      to_remove[SLIME_UTILS::STDOUT_IDX] = i;
    } else if ((args[i] == ">>" || args[i] == "1>>") && inbound) {
      info.stdout_append_file = args[i + 1];
      to_remove[SLIME_UTILS::STDOUT_APPEND_IDX] = i;
    } else if (args[i] == "2>" && inbound) {
      info.stderr_file = args[i + 1];
      to_remove[SLIME_UTILS::STDERR_IDX] = i;
    } else if (args[i] == "2>>" && inbound) {
      info.stderr_append_file = args[i + 1];
      to_remove[SLIME_UTILS::STDERR_APPEND_IDX] = i;
    }
  }
  // we want to reverse from the back to front to avoid shifting issues
  std::sort(to_remove.rbegin(), to_remove.rend());
  for (const int& i : to_remove) {
    if (i == -1) continue;
    args.erase(args.begin() + i, args.begin() + i + 2);
  }
  return info;
}

bool is_built_in(const std::string& command) noexcept {
  return CommandRegistry::IsBuiltIn(command);
}

char** autocomplete(const char* text, int start, int end) {
  rl_attempted_completion_over = 1;
  std::string match = CommandRegistry::autocomplete(text);
  if (match.empty()) return nullptr;

  char** arr = new char*[2];
  // converts the string to a heap allocated C string.
  // strdup does malloc + strcpy
  arr[0] = strdup(match.c_str());
  arr[1] = nullptr;
  return arr;
}

}  // namespace Slime
