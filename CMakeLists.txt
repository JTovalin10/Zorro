cmake_minimum_required(VERSION 3.13)

project(shell-starter-cpp)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ── Main shell binary ─────────────────────────────────────────────────────────

file(GLOB_RECURSE SHELL_SOURCES src/*.cpp src/*.hpp)

add_executable(shell ${SHELL_SOURCES})

target_include_directories(shell PRIVATE src /opt/homebrew/opt/readline/include)
target_link_directories(shell PRIVATE /opt/homebrew/opt/readline/lib)
target_link_libraries(shell PRIVATE readline)

# ── Shared library sources (everything except main.cpp and readline-heavy History) ──

file(GLOB_RECURSE ALL_SRC src/*.cpp)
list(FILTER ALL_SRC EXCLUDE REGEX ".*/main\\.cpp$")
list(FILTER ALL_SRC EXCLUDE REGEX ".*/History\\.cpp$")
list(FILTER ALL_SRC EXCLUDE REGEX ".*/HistoryCommand\\.cpp$")

# ── Unit tests ────────────────────────────────────────────────────────────────

add_executable(shell_tests
    ${ALL_SRC}
    tests/readline_stub.cpp
    tests/test_main.cpp
    tests/test_lexer.cpp
    tests/test_trie.cpp
    tests/test_exec.cpp
)

# tests/ directory must come BEFORE src/ so the readline stub headers shadow
# any system readline headers (or fill in for missing ones).
target_include_directories(shell_tests PRIVATE tests src)
# No -lreadline: we supply stubs.

# ── Throughput benchmarks ─────────────────────────────────────────────────────

add_executable(shell_bench
    ${ALL_SRC}
    tests/readline_stub.cpp
    tests/bench.cpp
)

target_include_directories(shell_bench PRIVATE tests src)
# bench.cpp uses open()/dup2() from fcntl.h — no extra link flags needed.
