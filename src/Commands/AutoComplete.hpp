#pragma once

#include <string>
#include <vector>

#include "Trie/Trie.hpp"

class AutoComplete {
 public:
  static void Add(const std::string& cmd) { GetTrie().insert(cmd.c_str()); }

  static std::vector<std::string> Run(const char* word) {
    return GetTrie().autocomplete(word);
  }

 private:
  static Trie& GetTrie() {
    static Trie instance{};
    return instance;
  }
};