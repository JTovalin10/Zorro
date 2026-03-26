#include "History.hpp"

#include <iostream>

namespace Slime {

History::History(const std::string& hist_file, const int max_entries)
    : g_hist_file(hist_file), g_max_entries(max_entries) {
  using_history();
  stifle_history(max_entries);
  read_history(g_hist_file.c_str());
  g_entries_start_at = history_length;
}

History::~History() { save(); }

void History::save() {
  // saves history to the file
  const char* file = g_hist_file.c_str();
  write_history(file);
  history_truncate_file(file, g_max_entries);
}

void History::remove(int n) {
  if (n < 0 || n >= history_length) return;
  HIST_ENTRY* entry = remove_history(n);

  if (!entry) return;

  free(entry->line);
  if (entry->data) free(entry->data);
  free(entry);
}

void History::read() { read_history(g_hist_file.c_str()); }

void History::clear() { clear_history(); }

void History::add(const std::string& line) { add_history(line.c_str()); }

void History::print(const int n) {
  HIST_ENTRY** list = history_list();
  // prints last n entries
  int num;
  if (n == 0) {
    num = 0;
  } else {
    num = (history_length > n) ? history_length - n : 0;
  }

  for (int i = num; list && list[i]; ++i) {
    std::cout << "    " << i + 1 << "  " << list[i]->line << '\n';
  }
  // n is up to n
}

}  // namespace Slime
