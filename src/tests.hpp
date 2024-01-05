#ifndef TESTS_H
#define TESTS_H
#include "logging.hpp"

#define TEST_ASSERT_MSG(cond, msg, ...) if (!(cond)) { ERROR(msg, __VA_ARGS__) return 1; }
#define TEST_ASSERT(cond) TEST_ASSERT_MSG(cond, "'%s' not satisfied", #cond)
#define TEST_ASSERT_EQUAL(x, y) TEST_ASSERT_MSG((x) == (y), "%s != %s", #x, #y)
#define TEST_ASSERT_STREQUAL(x, y) TEST_ASSERT_MSG(strcmp((x), (y)) == 0, "%s ('%s') != %s ('%s')", #x, x, #y, y)

int UnitTests();

#endif  // TESTS_H