#include "Trie.hpp"

#include <string>
#include <vector>

/**
 * finds the first word and returns it
 */
static std::string autocomplete_helper(TrieNode* node, std::string& current);

void Trie::insert(const char* word) {
  TrieNode* tmp = &root;
  for (int i{}; word[i] != '\0'; ++i) {
    if (!tmp->children.count(word[i])) {
      tmp->children[word[i]] = std::make_unique<TrieNode>();
    }
    tmp = tmp->children[word[i]].get();
  }
  tmp->end = true;
}

bool Trie::search(const char* word) {
  TrieNode* tmp = &root;
  for (int i{}; word[i] != '\0'; ++i) {
    if (!tmp->children.count(word[i])) return false;
    tmp = tmp->children[word[i]].get();
  }
  return tmp->end;
}

bool Trie::startsWith(const char* prefix) {
  TrieNode* tmp = &root;
  for (int i{}; prefix[i] != '\0'; ++i) {
    if (!tmp->children.count(prefix[i])) return false;
    tmp = tmp->children[prefix[i]].get();
  }
  return true;
}

std::string Trie::autocomplete(const char* prefix) {
  TrieNode* tmp = &root;
  for (int i{}; prefix[i] != '\0'; ++i) {
    if (!tmp->children.count(prefix[i])) return "";
    tmp = tmp->children[prefix[i]].get();
  }
  std::string current{prefix};
  return autocomplete_helper(tmp, current);
}

static std::string autocomplete_helper(TrieNode* node, std::string& current) {
  if (node->end) return current;

  for (auto& [ch, child] : node->children) {
    current.push_back(ch);
    std::string result = autocomplete_helper(child.get(), current);
    if (!result.empty()) return result;
    current.pop_back();  // backtrack
  }
  return "";
}
