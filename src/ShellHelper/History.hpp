#pragma once

#include <readline/history.h>
#include <readline/readline.h>

#include <filesystem>
#include <string>

namespace Slime {

class History {
 public:
  History(const std::string& hist_file = get_history_file(),
          const int max_entries = 500);

  ~History();

  History(const History& other) = delete;
  History(History&& other) = delete;
  History& operator=(const History& other) = delete;
  History& operator=(History&& other) = delete;

  /** reads history file into in-memory buffer */
  void read(const std::string& file);

  void append(const std::string& file);

  /**
   * Saves the current history to file
   */
  void save(const std::string& file);

  /**
   * removes entry n, if it exists
   */
  void remove(int n);

  /**
   * Clears the in memory history (not disk)
   */
  void clear();

  /**
   * Adds the current line to history
   */
  void add(const std::string& line);

  /**
   * prints n entries of the in memory history
   * If n > g_max_entries then it prints g_max_entries
   */
  void print(const int n = 0);

 private:
  std::string g_hist_file{};
  int g_entries_start_at{0};
  int g_max_entries{500};
  int last_appended{0};

  static const std::string get_history_file() {
    const char* path = std::getenv("HISTFILE");
    if (path) {
      return path;
    } else {
      return "~/.zorro_history";
    }
  }
};

inline History& get_history() {
  static History instance{};
  return instance;
}

}  // namespace Slime
