/* Tests for the state-machine lexer (parse_args) and dup2-based redirect
   parser (find_redirect).  These mirror the quoting/redirection stages that
   the CodeCrafters grader exercises on the shell binary. */
#include "framework.hpp"
#include "ShellHelper/ShellHelper.hpp"

#include <string>
#include <vector>

// ── parse_args ────────────────────────────────────────────────────────────────

static void test_plain_words() {
    Test::suite("parse_args – plain words");
    auto r = Slime::parse_args("echo hello world");
    Test::eq(r.size(), 3u, "token count");
    Test::eq(r[0], std::string("echo"),  "token[0]");
    Test::eq(r[1], std::string("hello"), "token[1]");
    Test::eq(r[2], std::string("world"), "token[2]");
}

static void test_single_quotes() {
    Test::suite("parse_args – single quotes");
    // Interior spaces must be preserved
    auto r = Slime::parse_args("echo 'hello world'");
    Test::eq(r.size(), 2u,                      "token count");
    Test::eq(r[1], std::string("hello world"),  "quoted arg");

    // Special chars inside single quotes are literal
    auto r2 = Slime::parse_args("echo '\\n$var'");
    Test::eq(r2[1], std::string("\\n$var"), "backslash literal in single quotes");

    // Empty single-quoted string '' is dropped (no empty-token production)
    auto r3 = Slime::parse_args("echo ''");
    Test::eq(r3.size(), 1u, "empty single quotes produces no token");
}

static void test_double_quotes() {
    Test::suite("parse_args – double quotes");
    // Interior spaces preserved
    auto r = Slime::parse_args(R"(echo "hello world")");
    Test::eq(r.size(), 2u,                     "token count");
    Test::eq(r[1], std::string("hello world"), "double-quoted arg");

    // Escaped double quote inside double quotes
    auto r2 = Slime::parse_args(R"(echo "say \"hi\"")");
    Test::eq(r2[1], std::string(R"(say "hi")"), "escaped dquote inside dquotes");

    // Escaped backslash inside double quotes
    auto r3 = Slime::parse_args(R"(echo "a\\b")");
    Test::eq(r3[1], std::string("a\\b"), "escaped backslash inside dquotes");

    // Other backslash sequences are literal inside double quotes
    auto r4 = Slime::parse_args(R"(echo "\n")");
    Test::eq(r4[1], std::string("\\n"), "non-special backslash sequence in dquotes");
}

static void test_backslash_normal() {
    Test::suite("parse_args – backslash outside quotes");
    // Backslash escapes the next character
    auto r = Slime::parse_args("echo he\\llo");
    Test::eq(r[1], std::string("hello"), "backslash-escaped char in normal state");

    auto r2 = Slime::parse_args("echo a\\ b");
    Test::eq(r2.size(), 2u,             "backslash-escaped space keeps tokens together");
    Test::eq(r2[1], std::string("a b"), "backslash-space produces literal space");
}

static void test_mixed_quoting() {
    Test::suite("parse_args – mixed quoting");
    auto r = Slime::parse_args("echo 'foo'\"bar\"");
    Test::eq(r.size(), 2u,              "token count");
    Test::eq(r[1], std::string("foobar"), "adjacent quoted strings concatenate");
}

static void test_multiple_spaces() {
    Test::suite("parse_args – multiple spaces");
    auto r = Slime::parse_args("echo  a   b");
    Test::eq(r.size(), 3u, "extra spaces between tokens ignored");
}

// ── find_redirect ─────────────────────────────────────────────────────────────

static void test_stdout_redirect() {
    Test::suite("find_redirect – stdout >");
    std::vector<std::string> args{"echo", "hello", ">", "out.txt"};
    auto info = Slime::find_redirect(args);
    Test::eq(info.stdout_file,  std::string("out.txt"), "stdout_file captured");
    Test::eq(info.stderr_file,  std::string(""),        "stderr_file empty");
    Test::eq(args.size(), 2u,                           "redirect tokens stripped");
    Test::eq(args[0], std::string("echo"),              "command preserved");
}

static void test_stdout_redirect_1arrow() {
    Test::suite("find_redirect – stdout 1>");
    std::vector<std::string> args{"ls", "1>", "out.txt"};
    auto info = Slime::find_redirect(args);
    Test::eq(info.stdout_file, std::string("out.txt"), "1> recognised as stdout");
}

static void test_stdout_append() {
    Test::suite("find_redirect – stdout append >>");
    std::vector<std::string> args{"echo", "hi", ">>", "log.txt"};
    auto info = Slime::find_redirect(args);
    Test::eq(info.astdout_file, std::string("log.txt"), "astdout_file captured");
    Test::check(info.stdout_file.empty(),               "stdout_file empty");
}

static void test_stderr_redirect() {
    Test::suite("find_redirect – stderr 2>");
    std::vector<std::string> args{"ls", "/nope", "2>", "err.txt"};
    auto info = Slime::find_redirect(args);
    Test::eq(info.stderr_file, std::string("err.txt"), "stderr_file captured");
    Test::check(info.stdout_file.empty(),               "stdout_file empty");
}

static void test_stderr_append() {
    Test::suite("find_redirect – stderr append 2>>");
    std::vector<std::string> args{"cmd", "2>>", "err.log"};
    auto info = Slime::find_redirect(args);
    Test::eq(info.astderr_file, std::string("err.log"), "astderr_file captured");
}

static void test_combined_redirects() {
    Test::suite("find_redirect – combined stdout + stderr");
    std::vector<std::string> args{"cmd", ">", "out.txt", "2>", "err.txt"};
    auto info = Slime::find_redirect(args);
    Test::eq(info.stdout_file, std::string("out.txt"), "stdout captured");
    Test::eq(info.stderr_file, std::string("err.txt"), "stderr captured");
    Test::eq(args.size(), 1u,                          "all redirect tokens stripped");
}

static void test_no_redirect() {
    Test::suite("find_redirect – no redirect");
    std::vector<std::string> args{"echo", "hello"};
    auto info = Slime::find_redirect(args);
    Test::check(!info.has_any(),    "has_any() false");
    Test::eq(args.size(), 2u,      "args unchanged");
}

static void test_redirect_args_remaining() {
    Test::suite("find_redirect – extra args survive stripping");
    // ls > out.txt hello  →  command sees ["ls", "hello"]
    std::vector<std::string> args{"ls", ">", "out.txt", "hello"};
    auto info = Slime::find_redirect(args);
    Test::eq(info.stdout_file, std::string("out.txt"), "stdout_file");
    Test::eq(args.size(), 2u,                          "remaining token count");
    Test::eq(args[1], std::string("hello"),            "extra arg preserved");
}

// ── find_pipe ─────────────────────────────────────────────────────────────────

static void test_no_pipe() {
    Test::suite("find_pipe – no pipe");
    std::vector<std::string> args{"echo", "hello"};
    auto segs = Slime::find_pipe(args);
    Test::eq(segs.size(), 1u,                "single segment");
    Test::eq(segs[0], std::string("echo hello"), "segment text");
}

static void test_one_pipe() {
    Test::suite("find_pipe – one pipe");
    std::vector<std::string> args{"echo", "hello", "|", "cat"};
    auto segs = Slime::find_pipe(args);
    Test::eq(segs.size(), 2u,                   "two segments");
    Test::eq(segs[0], std::string("echo hello"), "left segment");
    Test::eq(segs[1], std::string("cat"),         "right segment");
}

static void test_two_pipes() {
    Test::suite("find_pipe – two pipes");
    std::vector<std::string> args{"echo", "x", "|", "tr", "x", "y", "|", "cat"};
    auto segs = Slime::find_pipe(args);
    Test::eq(segs.size(), 3u, "three segments");
}

// ── public entry point called by test_main.cpp ────────────────────────────────

void run_lexer_tests() {
    test_plain_words();
    test_single_quotes();
    test_double_quotes();
    test_backslash_normal();
    test_mixed_quoting();
    test_multiple_spaces();
    test_stdout_redirect();
    test_stdout_redirect_1arrow();
    test_stdout_append();
    test_stderr_redirect();
    test_stderr_append();
    test_combined_redirects();
    test_no_redirect();
    test_redirect_args_remaining();
    test_no_pipe();
    test_one_pipe();
    test_two_pipes();
}
