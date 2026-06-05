/* Tests for the trie-backed autocomplete engine.
   Covers Trie::insert, search, startsWith, autocomplete, and clear; also
   verifies the AutoComplete / FileAutoComplete singleton wrappers that the
   shell uses for tab-completion via GNU Readline. */
#include "framework.hpp"
#include "Commands/Trie/Trie.hpp"
#include "Commands/AutoComplete.hpp"
#include "Commands/FileAutoComplete.hpp"

#include <algorithm>
#include <string>
#include <vector>

// ── Trie::insert / search / startsWith ───────────────────────────────────────

static void test_insert_search() {
    Test::suite("Trie – insert / search");
    Trie t;
    t.insert("echo");
    t.insert("exit");
    t.insert("export");

    Test::check(t.search("echo"),   "search exact match 'echo'");
    Test::check(t.search("exit"),   "search exact match 'exit'");
    Test::check(t.search("export"), "search exact match 'export'");
    Test::check(!t.search("ec"),    "search prefix only → false");
    Test::check(!t.search(""),      "search empty string → false");
    Test::check(!t.search("xyz"),   "search absent word → false");
}

static void test_starts_with() {
    Test::suite("Trie – startsWith");
    Trie t;
    t.insert("echo");
    t.insert("exit");

    Test::check(t.startsWith("ec"),  "prefix 'ec'");
    Test::check(t.startsWith("ex"),  "prefix 'ex'");
    Test::check(t.startsWith("e"),   "single char prefix");
    Test::check(t.startsWith("echo"),"full word as prefix");
    Test::check(!t.startsWith("ls"), "absent prefix");
    // empty prefix "" always returns true (loop body never executes)
    Test::check(t.startsWith(""),   "empty prefix → true (vacuously)");
}

static void test_autocomplete_single() {
    Test::suite("Trie – autocomplete single match");
    Trie t;
    t.insert("echo");
    t.insert("ls");

    auto r = t.autocomplete("ec");
    Test::eq(r.size(), 1u,                    "one completion");
    Test::eq(r[0],     std::string("echo"),   "correct completion");

    auto r2 = t.autocomplete("l");
    Test::eq(r2.size(), 1u,                   "one completion for 'l'");
    Test::eq(r2[0],     std::string("ls"),    "correct completion");
}

static void test_autocomplete_multiple() {
    Test::suite("Trie – autocomplete multiple matches");
    Trie t;
    t.insert("echo");
    t.insert("exit");
    t.insert("export");
    t.insert("env");

    auto r = t.autocomplete("e");
    Test::eq(r.size(), 4u, "four completions for 'e'");

    auto r2 = t.autocomplete("ex");
    // "echo" starts with "ec", so "ex" matches only "exit" and "export"
    Test::eq(r2.size(), 2u, "two completions for 'ex' (exit, export)");

    // results must be sorted
    auto sorted = r2;
    std::sort(sorted.begin(), sorted.end());
    Test::check(r2 == sorted, "autocomplete results are sorted");
}

static void test_autocomplete_no_match() {
    Test::suite("Trie – autocomplete no match");
    Trie t;
    t.insert("echo");

    auto r = t.autocomplete("xyz");
    Test::check(r.empty(), "empty vector when no prefix match");

    auto r2 = t.autocomplete("");
    // empty prefix → implementation-defined, but must not crash
    Test::check(true, "autocomplete('') does not crash");
}

static void test_autocomplete_exact_is_included() {
    Test::suite("Trie – autocomplete includes the exact word itself");
    Trie t;
    t.insert("echo");
    t.insert("echoes");

    auto r = t.autocomplete("echo");
    Test::check(r.size() >= 1u, "at least one result");
    bool has_echo = std::find(r.begin(), r.end(), "echo") != r.end();
    Test::check(has_echo, "exact word 'echo' in completions");
}

static void test_clear() {
    Test::suite("Trie – clear");
    Trie t;
    t.insert("echo");
    t.insert("ls");
    t.clear();

    Test::check(!t.search("echo"),      "search after clear → false");
    Test::check(!t.startsWith("ec"),    "startsWith after clear → false");
    Test::check(t.autocomplete("e").empty(), "autocomplete after clear → empty");
}

// ── AutoComplete singleton ────────────────────────────────────────────────────

static void test_autocomplete_singleton() {
    Test::suite("AutoComplete singleton");
    // The singleton trie is shared; existing shell commands may already be in it.
    // We insert a known-unique word and verify it's found.
    const std::string sentinel = "zorro_test_unique_cmd_xyz";
    AutoComplete::Add(sentinel);

    auto r = AutoComplete::Run("zorro_test_unique");
    bool found = !r.empty() && std::find(r.begin(), r.end(), sentinel) != r.end();
    Test::check(found, "AutoComplete::Run finds word added via Add()");
}

// ── FileAutoComplete singleton ────────────────────────────────────────────────

static void test_file_autocomplete_singleton() {
    Test::suite("FileAutoComplete singleton");
    FileAutoComplete::Clear();
    FileAutoComplete::Add("main.cpp");
    FileAutoComplete::Add("main.hpp");
    FileAutoComplete::Add("utils.cpp");

    auto r = FileAutoComplete::Run("main");
    Test::eq(r.size(), 2u, "two completions for 'main'");

    auto r2 = FileAutoComplete::Run("utils");
    Test::eq(r2.size(), 1u, "one completion for 'utils'");
    Test::eq(r2[0], std::string("utils.cpp"), "correct file completion");

    FileAutoComplete::Clear();
    auto r3 = FileAutoComplete::Run("main");
    Test::check(r3.empty(), "empty after Clear()");
}

// ── large-vocabulary correctness ─────────────────────────────────────────────

static void test_large_vocabulary() {
    Test::suite("Trie – large vocabulary (1 000 words)");
    Trie t;
    std::vector<std::string> words;
    words.reserve(1000);
    for (int i = 0; i < 1000; ++i) {
        words.push_back("cmd" + std::to_string(i));
        t.insert(words.back().c_str());
    }

    // All words are searchable
    bool all_found = true;
    for (const auto& w : words) {
        if (!t.search(w.c_str())) { all_found = false; break; }
    }
    Test::check(all_found, "all 1000 words found via search()");

    // Prefix "cmd1" should return cmd1, cmd10..cmd19, cmd100..cmd199 = 111 words
    auto r = t.autocomplete("cmd1");
    Test::check(r.size() == 111u, "autocomplete('cmd1') returns 111 words");
}

// ── public entry point ────────────────────────────────────────────────────────

void run_trie_tests() {
    test_insert_search();
    test_starts_with();
    test_autocomplete_single();
    test_autocomplete_multiple();
    test_autocomplete_no_match();
    test_autocomplete_exact_is_included();
    test_clear();
    test_autocomplete_singleton();
    test_file_autocomplete_singleton();
    test_large_vocabulary();
}
