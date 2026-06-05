#pragma once
#include <iostream>
#include <sstream>
#include <string>

/* Minimal test framework — no external dependencies. */
namespace Test {

inline int& _passes()   { static int n = 0; return n; }
inline int& _failures() { static int n = 0; return n; }

inline void suite(const std::string& name) {
    std::cout << "\n--- " << name << " ---" << std::endl;
}

inline void _record(bool ok, const std::string& label, const std::string& detail) {
    if (ok) {
        ++_passes();
        std::cout << "  PASS  " << label << std::endl;  // flush so fork() can't duplicate buffered data
    } else {
        ++_failures();
        std::cout << "  FAIL  " << label;
        if (!detail.empty()) std::cout << "  |  " << detail;
        std::cout << std::endl;
    }
}

inline void check(bool cond, const std::string& label, const std::string& detail = "") {
    _record(cond, label, detail);
}

template <typename A, typename B>
void eq(const A& got, const B& want, const std::string& label) {
    if (got == want) {
        _record(true, label, "");
    } else {
        std::ostringstream ss;
        ss << "got=<" << got << "> want=<" << want << ">";
        _record(false, label, ss.str());
    }
}

/* Print totals; returns 0 if all passed, 1 otherwise. */
inline int summary() {
    int total = _passes() + _failures();
    std::cout << "\n" << _passes() << "/" << total << " tests passed";
    if (_failures()) std::cout << "  (" << _failures() << " FAILED)";
    std::cout << "\n";
    return _failures() ? 1 : 0;
}

} // namespace Test
