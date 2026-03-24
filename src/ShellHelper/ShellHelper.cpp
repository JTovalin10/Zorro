#include "ShellHelper.hpp"

#include <fcntl.h>
#include <unistd.h>

#include <algorithm>
#include <cstddef>
#include <iostream>

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

static void redirect_stdout(const std::string& file) {
  int fd = open(file.c_str(), O_WRONLY | O_TRUNC | O_CREAT, 0644);
  dup2(fd, STDOUT_FILENO);
  close(fd);
}

static void append_stdout(const std::string& file) {
  int fd = open(file.c_str(), O_WRONLY | O_APPEND | O_CREAT, 0644);
  dup2(fd, STDOUT_FILENO);
  close(fd);
}

static void redirect_stderr(const std::string& file) {
  int fd = open(file.c_str(), O_WRONLY | O_TRUNC | O_CREAT, 0644);
  dup2(fd, STDERR_FILENO);
  close(fd);
}

static void append_stderr(const std::string& file) {
  int fd = open(file.c_str(), O_WRONLY | O_APPEND | O_CREAT, 0644);
  dup2(fd, STDERR_FILENO);
  close(fd);
}

using RedirectFn = void (*)(const std::string&);
using Entry = std::pair<const std::string&, RedirectFn>;

namespace Slime {

void RedirectInfo::apply() const {
  const Entry actions[] = {
      {stdout_file, redirect_stdout},
      {astdout_file, append_stdout},
      {stderr_file, redirect_stderr},
      {astderr_file, append_stderr},
  };
  for (const auto& [file, fn] : actions) {
    if (!file.empty()) fn(file);
  }
}

bool RedirectInfo::has_any() const {
  return !stdout_file.empty() || !astdout_file.empty() ||
         !stderr_file.empty() || !astderr_file.empty();
}

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
      info.astdout_file = args[i + 1];
      to_remove[SLIME_UTILS::STDOUT_APPEND_IDX] = i;
    } else if (args[i] == "2>" && inbound) {
      info.stderr_file = args[i + 1];
      to_remove[SLIME_UTILS::STDERR_IDX] = i;
    } else if (args[i] == "2>>" && inbound) {
      info.astderr_file = args[i + 1];
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

static bool is_prefix(std::vector<std::string> matches, int index) {
  std::string check = matches[0].substr(0, index);
  for (int i{1}; i < matches.size(); ++i) {
    if (matches[i].find(check) != 0) return false;
  }
  return true;
}

static std::string longest_common_prefix(std::vector<std::string> matches) {
  if (matches.size() == 0) return "";
  int l{1};
  int r = std::numeric_limits<int>::max();
  // find the shortest length string
  for (const auto& s : matches) {
    r = std::min(static_cast<size_t>(r), s.size());
  }
  while (l < r) {
    int mid = l + (r - l + 1) / 2;
    if (is_prefix(matches, mid)) {
      l = mid;
    } else {
      r = mid - 1;
    }
  }
  return matches[0].substr(0, r);
}

char** autocomplete(const char* text, int start, int end) {
  rl_attempted_completion_over = 1;
  std::vector<std::string> match =
      (start == 0) ? AutoComplete::Run(text) : FileAutoComplete::Run(text);

  if (match.empty()) {
    rl_ding();
    return nullptr;
  }

  std::string result = match[0];
  if (match.size() > 1) {
    result = longest_common_prefix(match);
    rl_completion_append_character = '\0';
    if (result == std::string(text)) {
      // retry: strip exact match and recompute if there are extending matches
      auto it = std::find(match.begin(), match.end(), std::string(text));
      if (it != match.end() && match.size() > 1) {
        match.erase(it);
        result = (match.size() == 1) ? match[0] : longest_common_prefix(match);
      }
    }
    if (result == std::string(text)) {
      static std::string last_ding_text;
      if (last_ding_text == std::string(text)) {
        // second TAB: display all completions
        std::cout << "\n";
        for (size_t i = 0; i < match.size(); ++i) {
          if (i > 0) std::cout << "  ";
          std::cout << match[i];
        }
        std::cout << "\n";
        rl_on_new_line();
        rl_redisplay();
        last_ding_text.clear();
      } else {
        // first TAB: ring bell and remember
        last_ding_text = std::string(text);
        rl_ding();
      }
      return nullptr;
    }
  }

  // set append character based on final result
  if (!result.empty() && result.back() == '/') {
    rl_completion_append_character = '\0';  // directory: no space
  } else if (match.size() == 1) {
    rl_completion_append_character = ' ';   // unambiguous match: add space
  } else {
    rl_completion_append_character = '\0';  // partial prefix: no space
  }

  char** arr = new char*[2];
  arr[0] = strdup(result.c_str());
  arr[1] = nullptr;
  return arr;
}

}  // namespace Slime
