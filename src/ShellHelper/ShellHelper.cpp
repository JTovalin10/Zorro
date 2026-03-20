#include "ShellHelper.hpp"

#include <cstddef>

#include "Commands/BuiltInCommand.hpp"

enum class STATE { NORMAL, SINGLE_QUOTE, DOUBLE_QUOTE, BACKSLASH };

static constexpr char squote = '\'';
static constexpr char dquote = '"';
static constexpr char bslash = '\\';
static constexpr char space = ' ';

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
        if (curr == squote) {
          state = STATE::SINGLE_QUOTE;
        } else if (curr == dquote) {
          state = STATE::DOUBLE_QUOTE;
        } else if (curr == bslash) {
          state = STATE::BACKSLASH;
        } else if (curr == space) {
          if (!scurr.empty()) {
            parsed_args.push_back(scurr);
            scurr.clear();
          }
        } else {
          scurr += curr;
        }
        break;
      case STATE::SINGLE_QUOTE:
        if (curr == squote) {
          state = STATE::NORMAL;
        } else {
          scurr += curr;
        }
        break;
      case STATE::DOUBLE_QUOTE:
        if (curr == dquote) {
          state = STATE::NORMAL;
        } else if (curr == bslash && i + 1 < size) {
          char next = user_args[i + 1];
          if (next == dquote || next == bslash) {
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
std::string find_redirect(std::vector<std::string>& args) {
  std::string file{};
  // removes i and i + 1
  int index_to_remove{-1};
  for (size_t i{}; i < args.size(); ++i) {
    if ((args[i] == ">" || args[i] == "1>") && i + 1 < args.size()) {
      file = args[i + 1];
      index_to_remove = i;
    }
  }
  if (index_to_remove != -1) {
    args.erase(args.begin() + index_to_remove,
               args.begin() + index_to_remove + 2);
  }
  return file;
}

bool is_built_in(const std::string& command) noexcept {
  return CommandRegistry::IsBuiltIn(command);
}

}  // namespace Slime
