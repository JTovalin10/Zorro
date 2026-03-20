#pragma once

#include <memory>
#include <unordered_map>

struct TrieNode {
  std::unordered_map<char, std::unique_ptr<TrieNode>> children{};
  bool end{false};
};

class Trie {
 public:
  /**
   * inserts the word into the trie
   *
   * ARGS:
   * word - the word to be inserted into the trie
   *
   */
  void insert(const char* word);

  /**
   * checks the trie to see if the entire word is in the trie
   *
   * ARGS:
   * word - the word to be searched
   *
   * RETURNS:
   * if the entire word is in the trie return true, else false
   */
  bool search(const char* word);

  /**
   * checks if the trie contains the word
   *
   * ARGS:
   * prefix - the prefix to search in the trie
   *
   * RETURNS:
   * if the prefix is in the trie return true
   * otherwise, return false
   */
  bool startsWith(const char* prefix);

  /**
   * auto completes the given word (if it can be)
   *
   * ARGS:
   * word - the word to be autocompleted
   *
   * RETURNS:
   * returns the finished word, or ''
   * ex: ec -> echo
   */
  std::string autocomplete(const char* word);

 private:
  TrieNode root;
};