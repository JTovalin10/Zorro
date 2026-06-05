/* Tests for fork/exec lifecycle and built-in dispatch.
   Covers is_built_in, is_executable, find_in_path, get_directories, and the
   full execa() path for both built-in and external commands. */
#include "framework.hpp"
#include "FileSys/FileSys.hpp"
#include "ShellHelper/ShellHelper.hpp"
#include "Commands/BuiltInCommand.hpp"

#include <unistd.h>
#include <string>
#include <vector>
#include <cstring>

// ── Helpers ───────────────────────────────────────────────────────────────────

/* Redirect stdout to a pipe, run fn(), restore, return captured text. */
static std::string capture(std::function<void()> fn) {
    std::cout.flush();   // drain iostream buffer before changing fd
    int pipefd[2];
    pipe(pipefd);
    int saved = dup(STDOUT_FILENO);
    dup2(pipefd[1], STDOUT_FILENO);
    close(pipefd[1]);

    fn();
    fflush(stdout);

    dup2(saved, STDOUT_FILENO);
    close(saved);

    char buf[65536] = {};
    ssize_t n = read(pipefd[0], buf, sizeof(buf) - 1);
    close(pipefd[0]);
    if (n < 0) n = 0;
    buf[n] = '\0';
    return std::string(buf);
}

/* Same but captures stderr. */
static std::string capture_err(std::function<void()> fn) {
    std::cerr.flush();
    int pipefd[2];
    pipe(pipefd);
    int saved = dup(STDERR_FILENO);
    dup2(pipefd[1], STDERR_FILENO);
    close(pipefd[1]);

    fn();
    fflush(stderr);

    dup2(saved, STDERR_FILENO);
    close(saved);

    char buf[65536] = {};
    ssize_t n = read(pipefd[0], buf, sizeof(buf) - 1);
    close(pipefd[0]);
    if (n < 0) n = 0;
    buf[n] = '\0';
    return std::string(buf);
}

// ── PATH utilities ────────────────────────────────────────────────────────────

static void test_get_directories() {
    Test::suite("get_directories");
    auto dirs = Slime::get_directories("/usr/bin:/usr/local/bin:/bin");
    Test::eq(dirs.size(), 3u,                       "three dirs");
    Test::eq(dirs[0], std::string("/usr/bin"),       "first dir");
    Test::eq(dirs[1], std::string("/usr/local/bin"), "second dir");
    Test::eq(dirs[2], std::string("/bin"),           "third dir");

    auto empty = Slime::get_directories(nullptr);
    Test::check(empty.empty(), "nullptr path → empty");
}

static void test_find_in_path() {
    Test::suite("find_in_path");
    const char* path = std::getenv("PATH");
    if (!path) { Test::check(true, "PATH not set – skipped"); return; }

    // "true" exists on every POSIX system
    std::string loc = Slime::find_in_path("true", path);
    Test::check(!loc.empty(), "'true' found in PATH");

    std::string absent = Slime::find_in_path("__no_such_cmd_zorro__", path);
    Test::check(absent.empty(), "absent command → empty string");
}

static void test_is_executable() {
    Test::suite("is_executable");
    // System commands that must exist on Linux
    Test::check(Slime::is_executable("true"),  "'true' is executable");
    Test::check(Slime::is_executable("false"), "'false' is executable");
    Test::check(!Slime::is_executable("__zorro_ghost__"), "absent cmd → false");
}

// ── CommandRegistry / built-in dispatch ──────────────────────────────────────

static void test_is_built_in() {
    Test::suite("is_built_in / CommandRegistry");
    // These are registered by the static initialisers in their .cpp files
    Test::check(Slime::is_built_in("echo"),    "'echo' is built-in");
    Test::check(Slime::is_built_in("exit"),    "'exit' is built-in");
    Test::check(Slime::is_built_in("type"),    "'type' is built-in");
    Test::check(Slime::is_built_in("pwd"),     "'pwd' is built-in");
    Test::check(Slime::is_built_in("cd"),      "'cd' is built-in");
    Test::check(!Slime::is_built_in("ls"),     "'ls' is NOT built-in");
    Test::check(!Slime::is_built_in("grep"),   "'grep' is NOT built-in");
    Test::check(!Slime::is_built_in(""),       "empty string is NOT built-in");
}

// ── echo built-in execution ───────────────────────────────────────────────────

static void test_echo_exec() {
    Test::suite("execa – echo built-in");
    std::vector<std::string> args{"echo", "hello", "world"};
    std::string out = capture([&] { Slime::execa(args); });
    Test::eq(out, std::string("hello world\n"), "echo hello world");
}

static void test_echo_single_arg() {
    Test::suite("execa – echo single arg");
    std::vector<std::string> args{"echo", "zorro"};
    std::string out = capture([&] { Slime::execa(args); });
    Test::eq(out, std::string("zorro\n"), "echo zorro");
}

static void test_echo_no_args() {
    Test::suite("execa – echo no args");
    std::vector<std::string> args{"echo"};
    std::string out = capture([&] { Slime::execa(args); });
    Test::eq(out, std::string("\n"), "echo with no args prints newline");
}

// ── type built-in ────────────────────────────────────────────────────────────

static void test_type_builtin() {
    Test::suite("execa – type for built-in");
    std::vector<std::string> args{"type", "echo"};
    std::string out = capture([&] { Slime::execa(args); });
    Test::check(out.find("shell builtin") != std::string::npos,
                "type echo → shell builtin");
}

static void test_type_external() {
    Test::suite("execa – type for external command");
    std::vector<std::string> args{"type", "ls"};
    std::string out = capture([&] { Slime::execa(args); });
    // should print the path, e.g. "ls is /usr/bin/ls"
    Test::check(out.find("ls is") != std::string::npos, "type ls → path found");
}

static void test_type_not_found() {
    Test::suite("execa – type for unknown command");
    std::vector<std::string> args{"type", "__zorro_ghost__"};
    std::string out = capture([&] { Slime::execa(args); });
    Test::check(out.find("not found") != std::string::npos, "type unknown → not found");
}

// ── external command execution ────────────────────────────────────────────────

static void test_external_true() {
    Test::suite("execa – external 'true'");
    // 'true' exits 0 and produces no output; just verify no crash
    std::vector<std::string> args{"true"};
    bool threw = false;
    try { Slime::execa(args); } catch (...) { threw = true; }
    Test::check(!threw, "'true' runs without throwing");
}

static void test_external_echo() {
    Test::suite("execa – external /bin/echo (non-builtin path)");
    // Call with full path to bypass built-in registry
    std::string echo_path = Slime::find_in_file_system("echo");
    if (echo_path.empty()) {
        Test::check(true, "echo not found in PATH – skipped");
        return;
    }
    std::vector<std::string> args{echo_path, "ext_echo_test"};
    std::string out = capture([&] { Slime::execa(args); });
    Test::check(out.find("ext_echo_test") != std::string::npos,
                "external echo produces expected output");
}

// ── stdout redirection ────────────────────────────────────────────────────────

static void test_stdout_redirect_to_file() {
    Test::suite("execa – stdout > file");
    const char* tmpfile = "/tmp/zorro_test_stdout.txt";
    std::vector<std::string> args{"echo", "redirect_test", ">", tmpfile};
    Slime::execa(args);  // writes to file inside forked child

    // Read back the file
    FILE* f = fopen(tmpfile, "r");
    Test::check(f != nullptr, "output file created");
    if (f) {
        char buf[256] = {};
        fgets(buf, sizeof(buf), f);
        fclose(f);
        unlink(tmpfile);
        std::string got(buf);
        Test::check(got.find("redirect_test") != std::string::npos,
                    "redirect output matches");
    }
}

static void test_stderr_redirect_to_file() {
    Test::suite("execa – stderr 2> file");
    const char* tmpfile = "/tmp/zorro_test_stderr.txt";
    // 'ls' of a non-existent path writes to stderr
    std::vector<std::string> args{"ls", "/no_such_path_zorro", "2>", tmpfile};
    Slime::execa(args);

    FILE* f = fopen(tmpfile, "r");
    Test::check(f != nullptr, "stderr file created");
    if (f) {
        char buf[512] = {};
        fread(buf, 1, sizeof(buf)-1, f);
        fclose(f);
        unlink(tmpfile);
        Test::check(strlen(buf) > 0, "stderr file is non-empty");
    }
}

// ── pipe execution ────────────────────────────────────────────────────────────

static void test_pipe_echo_cat() {
    Test::suite("execa – pipe  echo | cat");
    std::vector<std::string> args{"echo", "pipe_test", "|", "cat"};
    std::string out = capture([&] { Slime::execa(args); });
    Test::check(out.find("pipe_test") != std::string::npos,
                "pipe: echo output reaches cat");
}

// ── public entry point ────────────────────────────────────────────────────────

void run_exec_tests() {
    test_get_directories();
    test_find_in_path();
    test_is_executable();
    test_is_built_in();
    test_echo_exec();
    test_echo_single_arg();
    test_echo_no_args();
    test_type_builtin();
    test_type_external();
    test_type_not_found();
    test_external_true();
    test_external_echo();
    test_stdout_redirect_to_file();
    test_stderr_redirect_to_file();
    test_pipe_echo_cat();
}
