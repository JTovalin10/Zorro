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

History::~History() { save(g_hist_file); }

void History::save(const std::string& file) {
  // saves history to the file
  const char* cfile = file.c_str();
  write_history(cfile);
  history_truncate_file(cfile, g_max_entries);
}

void History::append(const std::string& file) {
  int new_entries = history_length - last_appended;
  if (new_entries <= 0) return;

  append_history(new_entries, file.c_str());
  last_appended = history_length;
}

void History::remove(int n) {
  if (n < 0 || n >= history_length) return;
  HIST_ENTRY* entry = remove_history(n);

  if (!entry) return;

  free(entry->line);
  if (entry->data) free(entry->data);
  free(entry);
}

void History::read(const std::string& file) { read_history(file.c_str()); }

void History::clear() { clear_history(); }

void History::add(const std::string& line) { add_history(line.c_str()); }

void History::print(const int n) {
  HIST_ENTRY** list = history_list();
  // prints last n entries
  int num = (n == 0) ? g_entries_start_at
                     : std::max(g_entries_start_at, history_length - n);
  for (int i = num; list && list[i]; ++i) {
    std::cout << "    " << (i - g_entries_start_at + 1) << "  " << list[i]->line
              << '\n';
  }
  // n is up to n
}

}  // namespace Slime
