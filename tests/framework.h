#pragma once

#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <string>

// ─── namespace ────────────────────────────────────────────────────────────────

namespace test {

inline int s_pass = 0;
inline int s_fail = 0;

// ─── helpers ──────────────────────────────────────────────────────────────────

template <typename T>
inline std::string to_str(const T& v)
{
    std::ostringstream oss;
    oss << v;
    return oss.str();
}

// ─── core reporters ───────────────────────────────────────────────────────────

inline void pass(const char* name)
{
    ++s_pass;
    std::printf("\033[32m[PASS]\033[0m %s\n", name);
}

inline void fail(const char* name, const char* file, int line, const char* expr)
{
    ++s_fail;
    std::printf("\033[31m[FAIL]\033[0m %s @ %s:%d \xe2\x80\x94 %s\n", name, file, line, expr);
}

inline int summary()
{
    int total = s_pass + s_fail;
    if (s_fail == 0)
        std::printf("\033[32m%d tests: %d passed, %d failed\033[0m\n", total, s_pass, s_fail);
    else
        std::printf("\033[31m%d tests: %d passed, %d failed\033[0m\n", total, s_pass, s_fail);
    return (s_fail > 0) ? EXIT_FAILURE : EXIT_SUCCESS;
}

} // namespace test

// ─── macros ───────────────────────────────────────────────────────────────────

// ASSERT(expr) — fail and return if expr is false
#define ASSERT(expr)                                                \
    do {                                                            \
        if (!(expr)) {                                              \
            test::fail(__func__, __FILE__, __LINE__, #expr);        \
            return;                                                 \
        }                                                           \
    } while (0)

// ASSERT_TRUE(x) — alias for ASSERT
#define ASSERT_TRUE(x) ASSERT(x)

// ASSERT_FALSE(x) — fail and return if x is true
#define ASSERT_FALSE(x) ASSERT(!(x))

// ASSERT_EQ(a, b) — fail if a != b, printing expected/got
#define ASSERT_EQ(a, b)                                                         \
    do {                                                                        \
        auto const& _a = (a);                                                   \
        auto const& _b = (b);                                                   \
        if (!(_a == _b)) {                                                      \
            std::printf("  expected: %s\n  got:      %s\n",                    \
                        test::to_str(_b).c_str(),                               \
                        test::to_str(_a).c_str());                              \
            test::fail(__func__, __FILE__, __LINE__, #a " == " #b);             \
            return;                                                             \
        }                                                                       \
    } while (0)

// ASSERT_NE(a, b) — fail if a == b
#define ASSERT_NE(a, b)                                                         \
    do {                                                                        \
        auto const& _a = (a);                                                   \
        auto const& _b = (b);                                                   \
        if (_a == _b) {                                                         \
            std::printf("  expected: %s != %s\n  got:      both equal %s\n",   \
                        #a, #b,                                                 \
                        test::to_str(_a).c_str());                              \
            test::fail(__func__, __FILE__, __LINE__, #a " != " #b);             \
            return;                                                             \
        }                                                                       \
    } while (0)

// RUN_TEST(fn) — call fn(); pass(#fn) only if no new failures were recorded
#define RUN_TEST(fn)                                \
    do {                                            \
        int _prev_fail = test::s_fail;              \
        fn();                                       \
        if (test::s_fail == _prev_fail)             \
            test::pass(#fn);                        \
    } while (0)
