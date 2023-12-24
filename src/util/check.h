#ifndef __CHECK_H__
#define __CHECK_H__

#include <log.h>

#ifndef CHECKS_ENABLED
    #define CHECKS_ENABLED 1
#endif

#define FATAL(...)                \
    do {                          \
        LOG_FATAL(__VA_ARGS__);   \
        sdbox::PrintStackTrace(); \
        std::abort();             \
    } while (0)

// Quiet fatal: no stacktrace
#define QFATAL(...)             \
    do {                        \
        LOG_FATAL(__VA_ARGS__); \
        std::abort();           \
    } while (0)

#define CHECK(x)  (!(!(x) && (FATAL("CHECK: {}", #x), true)))
#define QCHECK(x) (!(!(x) && (QFATAL("CHECK: {}", #x), true)))

#define CHECK_EQ(a, b) CHECK_OP_IMPL(a, b, ==)
#define CHECK_NE(a, b) CHECK_OP_IMPL(a, b, !=)
#define CHECK_GT(a, b) CHECK_OP_IMPL(a, b, >)
#define CHECK_GE(a, b) CHECK_OP_IMPL(a, b, >=)
#define CHECK_LT(a, b) CHECK_OP_IMPL(a, b, <)
#define CHECK_LE(a, b) CHECK_OP_IMPL(a, b, <=)

// Quiet checks: no stacktrace
#define QCHECK_EQ(a, b) QCHECK_OP_IMPL(a, b, ==)
#define QCHECK_NE(a, b) QCHECK_OP_IMPL(a, b, !=)
#define QCHECK_GT(a, b) QCHECK_OP_IMPL(a, b, >)
#define QCHECK_GE(a, b) QCHECK_OP_IMPL(a, b, >=)
#define QCHECK_LT(a, b) QCHECK_OP_IMPL(a, b, <)
#define QCHECK_LE(a, b) QCHECK_OP_IMPL(a, b, <=)

#define CHECK_OP_IMPL(a, b, op)  GENERIC_CHECK_OP_IMPL(FATAL, a, b, op)
#define QCHECK_OP_IMPL(a, b, op) GENERIC_CHECK_OP_IMPL(QFATAL, a, b, op)

#if CHECKS_ENABLED
    #define GENERIC_CHECK_OP_IMPL(fatal, a, b, op)                                         \
        do {                                                                               \
            auto va = a;                                                                   \
            auto vb = b;                                                                   \
            if (!(va op vb))                                                               \
                fatal("CHECK: {0} " #op " {1} with {0} = {2}, {1} = {3}", #a, #b, va, vb); \
        } while (0)
#else
    #define GENERIC_CHECK_OP_IMPL(fatal, a, b, op) (void)0
#endif

#if defined(DEBUG) && CHECKS_ENABLED
    #define DCHECK(x) (CHECK(x))

    #define DCHECK_EQ(a, b) CHECK_EQ(a, b)
    #define DCHECK_NE(a, b) CHECK_NE(a, b)
    #define DCHECK_GT(a, b) CHECK_GT(a, b)
    #define DCHECK_GE(a, b) CHECK_GE(a, b)
    #define DCHECK_LT(a, b) CHECK_LT(a, b)
    #define DCHECK_LE(a, b) CHECK_LE(a, b)
#else
    #define DCHECK(x) (void)0

    #define DCHECK_EQ(a, b) (void)0
    #define DCHECK_NE(a, b) (void)0
    #define DCHECK_GT(a, b) (void)0
    #define DCHECK_GE(a, b) (void)0
    #define DCHECK_LT(a, b) (void)0
    #define DCHECK_LE(a, b) (void)0
#endif

#endif // __CHECK_H__