#include "Trie.hpp"

#include <algorithm>
/**
 * finds the first word and returns it
 */
static void autocomplete_helper(TrieNode* node, std::string& current,
                                std::vector<std::string>& result);

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

std::vector<std::string> Trie::autocomplete(const char* prefix) {
  TrieNode* tmp = &root;
  for (int i{}; prefix[i] != '\0'; ++i) {
    if (!tmp->children.count(prefix[i])) return {};
    tmp = tmp->children[prefix[i]].get();
  }
  std::vector<std::string> result{};
  std::string current{prefix};
  autocomplete_helper(tmp, current, result);
  std::sort(result.begin(), result.end());
  return result;
}

static void autocomplete_helper(TrieNode* node, std::string& current,
                                std::vector<std::string>& result) {
  if (node->end) result.push_back(current);

  for (auto& [ch, child] : node->children) {
    current.push_back(ch);
    autocomplete_helper(child.get(), current, result);
    current.pop_back();  // backtrack
  }
}
