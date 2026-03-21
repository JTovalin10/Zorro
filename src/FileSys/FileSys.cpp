#include "FileSys.hpp"

#include <fcntl.h>
#include <sys/stat.h>

#include <functional>
#include <ranges>

namespace Slime {

static void fork_and_run(std::function<void()> child_fn) {
  pid_t pid = fork();
  if (pid == 0) {
    child_fn();
    exit(0);
  } else {
    int status;
    waitpid(pid, &status, 0);
  }
}

static void run_external(const std::vector<std::string>& inputs) {
  std::vector<char*> argv;
  argv.reserve(inputs.size() + 1);
  for (const auto& s : inputs) {
    argv.push_back(const_cast<char*>(s.c_str()));
  }
  argv.push_back(nullptr);
  execvp(inputs[0].c_str(), argv.data());
}

void execb(std::vector<std::string>& inputs) {
  Slime::RedirectInfo redirect = Slime::find_redirect(inputs);
  if (redirect.has_any()) {
    fork_and_run([&] {
      redirect.apply();
      CommandRegistry::Run(inputs[0], inputs);
    });
  } else {
    CommandRegistry::Run(inputs[0], inputs);
  }
}

void execnb(std::vector<std::string>& inputs) {
  Slime::RedirectInfo redirect = Slime::find_redirect(inputs);
  fork_and_run([&] {
    redirect.apply();
    run_external(inputs);
  });
}

std::vector<std::string> get_directories(const char* char_path) {
  if (!char_path) return {};
  std::string_view path(char_path);
  std::vector<std::string> result;

  for (auto&& word : path | std::views::split(':')) {
    result.emplace_back(word.begin(), word.end());
  }
  return result;
}

bool check_file_permission_status(const fs::path& path) {
  auto status = fs::status(path);
  auto permissions = status.permissions();
  return (permissions & fs::perms::owner_exec) != fs::perms::none;
}

std::string find_in_path(const std::string& command, const char* path) {
  if (!path) {
    return "";
  }
  std::vector<std::string> directories = get_directories(path);

  for (const auto& dir : directories) {
    fs::path full_path = fs::path(dir) / command;
    if (fs::exists(full_path)) {
      if (!check_file_permission_status(full_path)) continue;
      return full_path.string();
    }
  }
  return "";
}

bool is_executable(const std::string& command) {
  const char* path = std::getenv("PATH");
  std::string full_path = find_in_path(command, path);
  return !full_path.empty();
}

std::string find_in_file_system(const std::string& command) noexcept {
  char* path = std::getenv("PATH");
  // base case
  return find_in_path(command, path);
}
std::vector<std::string> find_all_execnb() {
  std::vector<std::string> commands{};
  const char* path = std::getenv("PATH");
  for (const auto& dir : get_directories(path)) {
    if (!fs::is_directory(dir)) continue;
    for (const auto& dir_entry : fs::recursive_directory_iterator(dir)) {
      if (!fs::is_regular_file(dir_entry) || fs::is_symlink(dir_entry))
        continue;
      if (!check_file_permission_status(dir_entry)) continue;
      commands.push_back(dir_entry.path().filename().string());
    }
  }
  return commands;
}

}  // namespace Slime