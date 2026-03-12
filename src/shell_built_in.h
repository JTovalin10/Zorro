#ifndef SHELL_BUILT_IN_H_
#define SHELL_BUILT_IN_H_

#include <unordered_set>

class shell_hash_set {
  public:
  shell_hash_set() {
   set.insert("echo");
   set.insert("type");
   set.insert("exit");
  }

  ~shell_hash_set() = default;
  shell_hash_set(const shell_hash_set& other) = default;
  shell_hash_set(shell_hash_set&& other) = default;
  shell_hash_set& operator=(const shell_hash_set& other) = default;
  shell_hash_set& operator=(shell_hash_set&& other) = default;

  bool contains(const std::string& other) noexcept {
    return set.find(other);
  }
  private:
  std::unordered_set<std::string> set;
};

#endif // SHELL_BUILT_IN_H_
