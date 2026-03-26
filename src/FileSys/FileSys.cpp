#include "FileSys.hpp"

#include <fcntl.h>
#include <sys/stat.h>

#include <functional>
#include <ranges>

#include "Commands/BuiltInCommand.hpp"

namespace fs = std::filesystem;

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
  std::vector<std::string> segments = Slime::find_pipe(inputs);
  if (segments.size() == 1) {
    Slime::RedirectInfo redirect = Slime::find_redirect(inputs);
    if (redirect.has_any()) {
      fork_and_run([&] {
        redirect.apply();
        CommandRegistry::Run(inputs[0], inputs);
      });
    } else {
      CommandRegistry::Run(inputs[0], inputs);
    }
    return;
  }
  int prev_fd{-1};
  std::vector<pid_t> pids{};
  for (int i{0}; i < segments.size(); ++i) {
    // pipefd[0] = read, pipefd[1] = write
    // the function call alerady calls read and write
    int pipefd[2] = {-1, -1};
    const bool is_not_last = (i != segments.size() - 1);
    if (is_not_last && pipe(pipefd) < 0) {
      perror("Pipe");
      exit(EXIT_FAILURE);
    }
    std::vector<std::string> tokens = parse_args(segments[i]);
    Slime::RedirectInfo redirects = find_redirect(tokens);
    pid_t pid = fork();
    if (pid < 0) {
      perror("fork");
      exit(EXIT_FAILURE);
    }
    if (pid == 0) {
      if (is_not_last) {
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);
        close(pipefd[0]);
      }

      if (prev_fd != -1) {
        dup2(prev_fd, STDIN_FILENO);
        close(prev_fd);
      }
      if (redirects.has_any()) redirects.apply();
      CommandRegistry::Run(tokens[0], tokens);
      exit(EXIT_SUCCESS);
    } else {
      pids.push_back(pid);
      if (prev_fd != -1) close(prev_fd);
      if (is_not_last) {
        close(pipefd[1]);
        prev_fd = pipefd[0];
      }
    }
  }
  for (const auto& pid : pids) {
    waitpid(pid, nullptr, 0);
  }
}

void execnb(std::vector<std::string>& inputs) {
  std::vector<std::string> segments = Slime::find_pipe(inputs);
  if (segments.size() == 1) {
    Slime::RedirectInfo redirect = Slime::find_redirect(inputs);
    fork_and_run([&] {
      redirect.apply();
      run_external(inputs);
    });
    return;
  }

  int prev_fd{-1};
  std::vector<pid_t> pids;
  for (int i{0}; i < segments.size(); ++i) {
    const bool is_not_last = i < segments.size() - 1;
    int pipefd[2] = {-1, -1};

    std::vector<std::string> tokens = parse_args(segments[i]);
    Slime::RedirectInfo redirects = find_redirect(tokens);
    if (pipe(pipefd) < 0) {
      perror("Pipe");
      exit(EXIT_FAILURE);
    }

    pid_t pid = fork();
    if (pid < 0) {
      perror("Fork");
      exit(EXIT_FAILURE);
    }

    if (pid == 0) {
      if (is_not_last) {
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);
      }

      if (prev_fd != -1) {
        dup2(prev_fd, STDIN_FILENO);
        close(prev_fd);
      }
      if (redirects.has_any()) redirects.apply();
      run_external(tokens);
    } else {
      pids.push_back(pid);
      if (prev_fd != -1) close(prev_fd);
      if (is_not_last) {
        prev_fd = pipefd[0];
        close(pipefd[1]);
      }
    }
  }
  for (const auto& pid : pids) {
    waitpid(pid, nullptr, 0);
  }
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

void insert_files_in_trie() {
  for (const auto& entry : fs::recursive_directory_iterator(".")) {
    if (fs::is_regular_file(entry)) {
      FileAutoComplete::Add(entry.path().filename().string());
      FileAutoComplete::Add(
          entry.path().string().substr(2));  // we have to strip the ./
    } else if (fs::is_directory(entry)) {
      FileAutoComplete::Add(entry.path().string().substr(2) + "/");
    }
  }
}

}  // namespace Slime