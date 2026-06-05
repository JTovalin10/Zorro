/* Throughput benchmarks for the three headline subsystems:
 *
 *  1. State-machine lexer  – parse_args() + find_redirect()
 *  2. Trie autocomplete    – insert() and autocomplete()
 *  3. fork/exec lifecycle  – execa() round-trips
 *
 * Run:  ./shell_bench [--json]
 *
 * Default output is a human-readable table.
 * Pass --json to emit a JSON array for CI ingestion.
 */
#include "ShellHelper/ShellHelper.hpp"
#include "FileSys/FileSys.hpp"
#include "Commands/Trie/Trie.hpp"

#include <fcntl.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

using Clock = std::chrono::steady_clock;
using ns    = std::chrono::nanoseconds;

// ── Result type ───────────────────────────────────────────────────────────────

struct BenchResult {
    std::string name;
    std::string category;   // "lexer" | "trie" | "exec"
    uint64_t    iters;
    double      ns_per_op;
    double      ops_per_sec;
};

// ── Runner ────────────────────────────────────────────────────────────────────

template <typename Fn>
BenchResult bench(const std::string& name, const std::string& category,
                  uint64_t iters, Fn&& fn) {
    // Warm up
    for (int i = 0; i < 3; ++i) fn(0);

    auto t0 = Clock::now();
    for (uint64_t i = 0; i < iters; ++i) fn(i);
    auto t1 = Clock::now();

    double elapsed_ns  = static_cast<double>(
        std::chrono::duration_cast<ns>(t1 - t0).count());
    double ns_per_op   = elapsed_ns / static_cast<double>(iters);
    double ops_per_sec = 1e9 / ns_per_op;

    return {name, category, iters, ns_per_op, ops_per_sec};
}

// ── Sink to prevent dead-code elimination ─────────────────────────────────────

static volatile uint64_t sink = 0;

// ── 1. Lexer benchmarks ───────────────────────────────────────────────────────

static std::vector<BenchResult> bench_lexer() {
    std::vector<BenchResult> out;

    // Representative inputs that exercise all four STATE transitions
    static const std::string inputs[] = {
        "echo hello world",
        "echo 'single quoted string with spaces'",
        R"(echo "double \"quoted\" with escapes")",
        "echo back\\slash escape",
        "ls -la /tmp > /dev/null 2>&1",
        "grep -r 'pattern' . | head -20 | tail -5",
    };
    const int N = sizeof(inputs) / sizeof(inputs[0]);

    // parse_args throughput
    out.push_back(bench("parse_args (6 representative inputs)", "lexer", 200'000,
        [&](uint64_t i) {
            auto r = Slime::parse_args(inputs[i % N]);
            sink += r.size();
        }));

    // find_redirect throughput — input already tokenised
    static const std::vector<std::vector<std::string>> redirect_cases = {
        {"echo", "hello", ">",  "out.txt"},
        {"cmd",  "2>",    "err.txt"},
        {"ls",   ">>",    "log.txt"},
        {"cmd",  ">",     "o.txt", "2>", "e.txt"},
        {"echo", "plain"},
    };
    const int M = redirect_cases.size();

    out.push_back(bench("find_redirect (5 patterns)", "lexer", 500'000,
        [&](uint64_t i) {
            auto args = redirect_cases[i % M];   // copy each iteration
            auto info = Slime::find_redirect(args);
            sink += info.has_any() ? 1 : 0;
        }));

    // Combined: full parse → redirect strip (what the shell does per command)
    out.push_back(bench("parse_args + find_redirect (combined)", "lexer", 200'000,
        [&](uint64_t i) {
            auto tokens = Slime::parse_args(inputs[i % N]);
            auto info   = Slime::find_redirect(tokens);
            sink += tokens.size() + (info.has_any() ? 1 : 0);
        }));

    return out;
}

// ── 2. Trie benchmarks ────────────────────────────────────────────────────────

static std::vector<BenchResult> bench_trie() {
    std::vector<BenchResult> out;

    // Build a vocabulary of 1 000 realistic command names
    std::vector<std::string> vocab;
    vocab.reserve(1000);
    static const char* prefixes[] = {
        "git", "docker", "kubectl", "npm", "cargo", "go", "python3",
        "systemctl", "journalctl", "ssh", "scp", "rsync", "curl", "wget",
        "find", "grep", "awk", "sed", "sort", "uniq", "cut", "tr"
    };
    const int NP = sizeof(prefixes) / sizeof(prefixes[0]);
    for (int i = 0; i < 1000; ++i) {
        vocab.push_back(std::string(prefixes[i % NP]) + std::to_string(i));
    }

    // Insert throughput: measure repeated clear + insert
    out.push_back(bench("Trie::insert 1 000 words (per full reload)", "trie", 500,
        [&](uint64_t) {
            Trie t;
            for (const auto& w : vocab) t.insert(w.c_str());
            sink += 1;
        }));

    // Per-word insert throughput
    out.push_back(bench("Trie::insert single word (hot trie)", "trie", 1'000'000,
        [&](uint64_t i) {
            static Trie t;
            t.insert(vocab[i % vocab.size()].c_str());
            sink += 1;
        }));

    // search throughput on a warm trie
    {
        Trie warm;
        for (const auto& w : vocab) warm.insert(w.c_str());
        out.push_back(bench("Trie::search (warm 1k-word trie)", "trie", 2'000'000,
            [&](uint64_t i) {
                sink += warm.search(vocab[i % vocab.size()].c_str()) ? 1 : 0;
            }));

        out.push_back(bench("Trie::startsWith (warm 1k-word trie)", "trie", 2'000'000,
            [&](uint64_t i) {
                // query the 3-char prefix of each word
                const auto& w = vocab[i % vocab.size()];
                std::string pfx = w.substr(0, std::min<size_t>(w.size(), 4));
                sink += warm.startsWith(pfx.c_str()) ? 1 : 0;
            }));

        // autocomplete throughput — short prefix that returns ~50 matches
        out.push_back(bench("Trie::autocomplete prefix='git' (~45 matches)", "trie", 100'000,
            [&](uint64_t) {
                auto r = warm.autocomplete("git");
                sink += r.size();
            }));

        out.push_back(bench("Trie::autocomplete prefix='g' (broad)", "trie", 50'000,
            [&](uint64_t) {
                auto r = warm.autocomplete("g");
                sink += r.size();
            }));
    }

    return out;
}

// ── 3. Fork/exec benchmarks ───────────────────────────────────────────────────

static std::vector<BenchResult> bench_exec() {
    std::vector<BenchResult> out;

    // is_built_in lookup (pure hash-map, no fork)
    out.push_back(bench("is_built_in() dispatch (echo)", "exec", 5'000'000,
        [&](uint64_t) {
            sink += Slime::is_built_in("echo") ? 1 : 0;
        }));

    out.push_back(bench("is_built_in() dispatch (miss)", "exec", 5'000'000,
        [&](uint64_t) {
            sink += Slime::is_built_in("__no_cmd__") ? 1 : 0;
        }));

    // is_executable: PATH walk
    out.push_back(bench("is_executable('true') PATH walk", "exec", 10'000,
        [&](uint64_t) {
            sink += Slime::is_executable("true") ? 1 : 0;
        }));

    // echo built-in via execa (runs in parent process, no fork)
    // Suppress output by redirecting stdout to /dev/null for duration of bench
    {
        int devnull = open("/dev/null", O_WRONLY);
        int saved   = dup(STDOUT_FILENO);
        dup2(devnull, STDOUT_FILENO);
        close(devnull);

        out.push_back(bench("execa(['echo','x']) built-in dispatch (no fork)", "exec", 100'000,
            [&](uint64_t) {
                std::vector<std::string> args{"echo", "x"};
                Slime::execa(args);
            }));

        dup2(saved, STDOUT_FILENO);
        close(saved);
    }

    // External command via fork+exec (the most expensive path)
    // Use /usr/bin/true – minimal startup cost
    std::string true_path = Slime::find_in_file_system("true");
    if (!true_path.empty()) {
        out.push_back(bench("execa(['true']) fork+exec+wait round-trip", "exec", 1'000,
            [&](uint64_t) {
                std::vector<std::string> args{true_path};
                Slime::execa(args);
            }));
    }

    return out;
}

// ── Reporting ─────────────────────────────────────────────────────────────────

static void print_table(const std::vector<BenchResult>& all) {
    // Column widths
    const int W_NAME = 52;
    const int W_CAT  = 7;
    const int W_ITER = 11;
    const int W_NS   = 12;
    const int W_OPS  = 14;

    auto line = [&] {
        std::cout << std::string(W_NAME+W_CAT+W_ITER+W_NS+W_OPS+6, '-') << "\n";
    };

    std::cout << "\n=== Zorro Shell Throughput Benchmarks ===\n\n";
    line();
    std::cout << std::left
              << std::setw(W_NAME) << "Benchmark"
              << std::setw(W_CAT)  << "System"
              << std::right
              << std::setw(W_ITER) << "Iters"
              << std::setw(W_NS)   << "ns/op"
              << std::setw(W_OPS)  << "ops/sec"
              << "\n";
    line();

    std::string last_cat;
    for (const auto& r : all) {
        if (r.category != last_cat && !last_cat.empty()) line();
        last_cat = r.category;
        std::cout << std::left  << std::setw(W_NAME) << r.name
                  << std::setw(W_CAT)  << r.category
                  << std::right
                  << std::setw(W_ITER) << r.iters
                  << std::setw(W_NS)   << std::fixed << std::setprecision(1) << r.ns_per_op
                  << std::setw(W_OPS)  << std::fixed << std::setprecision(0) << r.ops_per_sec
                  << "\n";
    }
    line();
    std::cout << "(sink=" << sink << " — prevents dead-code elimination)\n\n";
}

static void print_json(const std::vector<BenchResult>& all) {
    std::cout << "[\n";
    for (size_t i = 0; i < all.size(); ++i) {
        const auto& r = all[i];
        std::cout << "  {\n"
                  << "    \"name\": \""     << r.name     << "\",\n"
                  << "    \"category\": \"" << r.category << "\",\n"
                  << "    \"iters\": "      << r.iters    << ",\n"
                  << "    \"ns_per_op\": "  << std::fixed << std::setprecision(2) << r.ns_per_op  << ",\n"
                  << "    \"ops_per_sec\": "<< std::fixed << std::setprecision(0) << r.ops_per_sec << "\n"
                  << "  }" << (i + 1 < all.size() ? "," : "") << "\n";
    }
    std::cout << "]\n";
}

// ── main ─────────────────────────────────────────────────────────────────────

int main(int argc, char* argv[]) {
    bool json = argc > 1 && std::strcmp(argv[1], "--json") == 0;

    std::vector<BenchResult> all;

    auto lexer = bench_lexer();
    all.insert(all.end(), lexer.begin(), lexer.end());

    auto trie = bench_trie();
    all.insert(all.end(), trie.begin(), trie.end());

    auto exec = bench_exec();
    all.insert(all.end(), exec.begin(), exec.end());

    if (json)
        print_json(all);
    else
        print_table(all);

    return 0;
}
