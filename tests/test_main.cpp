/* Entry point for the unit test executable. */
#include "framework.hpp"
#include <iostream>

void run_lexer_tests();
void run_trie_tests();
void run_exec_tests();

int main() {
    std::cout << "=== Zorro Shell Unit Tests ===\n";

    run_lexer_tests();
    run_trie_tests();
    run_exec_tests();

    return Test::summary();
}
