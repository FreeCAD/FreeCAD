#ifndef COIN_TESTSUITE_COINTTEST_H
#define COIN_TESTSUITE_COINTTEST_H

#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>

namespace CoinTest {

struct AbortTestCase { };

struct TestCase {
  const char * name;
  void (*fn)(void);
  const char * file;
  int line;
};

struct Failure {
  const char * file;
  int line;
  std::string message;
};

struct Context {
  int checks = 0;
  int failed = 0;
  std::vector<Failure> failures;
};

inline std::vector<TestCase> & registry(void)
{
  static std::vector<TestCase> r;
  return r;
}

inline Context *& current_context(void)
{
  static Context * ctx = NULL;
  return ctx;
}

struct Registrar {
  Registrar(const char * name, void (*fn)(void), const char * file, int line)
  {
    registry().push_back(TestCase{name, fn, file, line});
  }
};

inline void add_failure(const char * file, int line, const std::string & message)
{
  Context * ctx = current_context();
  if (!ctx) return;
  ctx->failed += 1;
  ctx->failures.push_back(Failure{file, line, message});
}

inline void add_check(void)
{
  Context * ctx = current_context();
  if (!ctx) return;
  ctx->checks += 1;
}

template <typename T>
std::string stringify(const T & v)
{
  std::ostringstream oss;
  oss << v;
  return oss.str();
}

inline void check(bool cond, const std::string & message, const char * file, int line)
{
  add_check();
  if (!cond) add_failure(file, line, message);
}

inline void require(bool cond, const std::string & message, const char * file, int line)
{
  add_check();
  if (!cond) {
    add_failure(file, line, message);
    throw AbortTestCase();
  }
}

template <typename A, typename B>
inline void check_equal(const A & a, const B & b,
                        const char * aexpr, const char * bexpr,
                        const char * file, int line)
{
  add_check();
  if (!(a == b)) {
    add_failure(file, line,
                std::string("check failed: (") + aexpr + " == " + bexpr + ") (" +
                  stringify(a) + " != " + stringify(b) + ")");
  }
}

inline int run_all(void)
{
  const std::vector<TestCase> & tests = registry();
  int failedtests = 0;
  int totalchecks = 0;

  for (size_t i = 0; i < tests.size(); ++i) {
    const TestCase & tc = tests[i];

    Context ctx;
    current_context() = &ctx;

    try {
      tc.fn();
    } catch (const AbortTestCase &) {
      // already recorded as failure(s)
    } catch (const std::exception & e) {
      add_failure(tc.file, tc.line, std::string("unhandled std::exception: ") + e.what());
    } catch (...) {
      add_failure(tc.file, tc.line, "unhandled non-std exception");
    }

    current_context() = NULL;
    totalchecks += ctx.checks;

    if (ctx.failed) {
      failedtests += 1;
      std::fprintf(stderr, "[FAIL] %s (%s:%d)\n", tc.name, tc.file, tc.line);
      for (size_t f = 0; f < ctx.failures.size(); ++f) {
        const Failure & fail = ctx.failures[f];
        std::fprintf(stderr, "  %s:%d: %s\n", fail.file, fail.line, fail.message.c_str());
      }
    }
  }

  if (failedtests == 0) {
    std::fprintf(stderr, "[OK] %zu tests, %d checks\n", tests.size(), totalchecks);
    return 0;
  }

  std::fprintf(stderr, "[FAIL] %d/%zu tests failed, %d checks\n", failedtests, tests.size(), totalchecks);
  return 1;
}

} // namespace CoinTest

// ---------------------------------------------------------------------------
// Minimal Boost.Test compatibility macros used by Coin's testsuite extractor
// ---------------------------------------------------------------------------

#define BOOST_TEST_NO_LIB 1

#define COIN_TEST_CONCAT_INNER(a, b) a##b
#define COIN_TEST_CONCAT(a, b) COIN_TEST_CONCAT_INNER(a, b)

#define BOOST_AUTO_TEST_SUITE(name) namespace name { enum { coin_testsuite_dummy = 0 };
#define BOOST_AUTO_TEST_SUITE_END() }
#define BOOST_AUTO_TEST_CASE_EXPECTED_FAILURES(name, n) /* no-op */

#define BOOST_AUTO_TEST_CASE(name)                                            \
  static void COIN_TEST_CONCAT(coin_test_fn_, __LINE__)(void);                \
  static ::CoinTest::Registrar COIN_TEST_CONCAT(coin_test_reg_, __LINE__)(    \
    #name, &COIN_TEST_CONCAT(coin_test_fn_, __LINE__), __FILE__, __LINE__);  \
  static void COIN_TEST_CONCAT(coin_test_fn_, __LINE__)(void)

#define BOOST_CHECK_MESSAGE(cond, msg) \
  ::CoinTest::check(!!(cond), (msg), __FILE__, __LINE__)

#define BOOST_CHECK(cond) \
  ::CoinTest::check(!!(cond), std::string("check failed: ") + #cond, __FILE__, __LINE__)

#define BOOST_CHECK_EQUAL(a, b) \
  ::CoinTest::check_equal((a), (b), #a, #b, __FILE__, __LINE__)

#define BOOST_REQUIRE_MESSAGE(cond, msg) \
  ::CoinTest::require(!!(cond), (msg), __FILE__, __LINE__)

#define BOOST_REQUIRE(cond) \
  ::CoinTest::require(!!(cond), std::string("require failed: ") + #cond, __FILE__, __LINE__)

#define BOOST_REQUIRE_EQUAL(a, b) \
  ::CoinTest::require(((a) == (b)), std::string("require failed: (") + #a + " == " + #b + ")", __FILE__, __LINE__)

#define BOOST_REQUIRE_NE(a, b) \
  ::CoinTest::require(((a) != (b)), std::string("require failed: (") + #a + " != " + #b + ")", __FILE__, __LINE__)

#define BOOST_ASSERT(cond) BOOST_REQUIRE(cond)

#define BOOST_REQUIRE_THROW(expr, exctype)                                    \
  do {                                                                        \
    bool coin_threw = false;                                                  \
    try { (void)(expr); } catch (const exctype &) { coin_threw = true; }      \
    ::CoinTest::require(coin_threw, std::string("expected throw: ") + #expr, __FILE__, __LINE__); \
  } while (0)

#define BOOST_STATIC_ASSERT(expr) static_assert((expr), #expr)

#endif // !COIN_TESTSUITE_COINTTEST_H
