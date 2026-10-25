# Zorro

A POSIX-style interactive shell written from scratch in C++23. It supports pipelines, I/O redirection, quoting, command history and tab completion. It uses `fork`, `execvp`, `pipe` and `dup2` directly instead of wrapping `system()`.

## Features

- Built-ins: `cd`, `pwd`, `echo`, `type`, `history`, `exit`
- Runs any external program found on `$PATH`
- Pipelines (`a | b | c`) that can mix built-ins and external programs
- Redirection: `>`, `>>`, `1>`, `1>>`, `2>`, `2>>`
- Single quotes, double quotes and backslash escapes
- Persistent history through GNU Readline (`$HISTFILE`, `history -r/-w/-a/-c/-d`)
- Tab completion for commands and files

## How it works

1. **Read:** `main` reads a line with Readline.
2. **Parse:** a hand-written state-machine lexer splits the line into tokens, handling quotes and escapes.
3. **Execute:** the line is split on `|`, redirects are extracted for each segment, and each segment is forked. Built-ins run through a `CommandRegistry`, and everything else goes to `execvp`.
4. **Complete:** a Trie indexes every executable on `$PATH` and every file in the working directory. Tab completes to the longest common prefix, and a second Tab lists all matches.

Each built-in subclasses `BuiltInCommand` and registers itself, so adding a command means adding one file under `src/Commands/`.

## Project layout

```
src/
  main.cpp                 REPL loop
  ShellHelper/             Lexer, redirect/pipe parsing, history
  FileSys/                 PATH lookup, fork/exec, pipelines
  Commands/                Built-ins, command registry, Trie autocomplete
tests/                     Unit tests, integration script, benchmarks
```

## Build and run

Requires CMake 3.13+, a C++23 compiler, and GNU Readline. `CMakeLists.txt` currently points at a Homebrew readline path, so Linux users may need to adjust it.

```sh
cmake -B build && cmake --build build
./build/shell              # run the shell
./build/shell_tests        # unit tests
./build/shell_bench        # benchmarks
./tests/integration.sh build/shell
```
