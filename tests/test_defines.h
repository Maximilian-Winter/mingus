#ifndef TEST_DEFINES_H
#define TEST_DEFINES_H

// Simple integer constants
#define TD_VERSION 1
#define TD_MAX_SIZE 1024
#define TD_NEGATIVE -42

// Hex constants
#define TD_FLAG_A 0x01
#define TD_FLAG_B 0x02
#define TD_FLAG_C 0x10
#define TD_BIG_HEX 0xDEADBEEF

// Unsigned constants
#define TD_UNSIGNED 100u
#define TD_BIG_UNSIGNED 0xFFFFFFFFu

// 64-bit constants
#define TD_LONG_VAL 0x100000000LL
#define TD_ULONG_VAL 0xFFFFFFFFFFFFFFFFULL

// Float constants
#define TD_PI 3.14159
#define TD_HALF 0.5f

// String constant
#define TD_NAME "TestDefines"

// Function-like macros (should be SKIPPED)
#define TD_ADD(a, b) ((a) + (b))
#define TD_MAX(x, y) ((x) > (y) ? (x) : (y))

// Expression macros (should be SKIPPED - conservative approach)
#define TD_COMBINED (TD_FLAG_A | TD_FLAG_B)

// Empty define / include guard (should be SKIPPED)
#define TD_EMPTY

// Simple function for end-to-end test
int td_get_version(void);
int td_add(int a, int b);

#endif
