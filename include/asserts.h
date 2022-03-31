
#ifdef DEBUG
#define DEBUG_ASSERT(__cond) assert(__cond)
#define DEBUG_ASSERT_MSG(__cond, __msg) assert((__cond) && (__msg))
#define LOG_ERROR(fmtstring, ...) fprintf(stderr, "[%s] " fmtstring, __file_logging_name, ##__VA_ARGS__)
#define LOG_ERROR_UNPREFIXED(fmtstring, ...) fprintf(stderr, fmtstring, ##__VA_ARGS__)
#define LOG_DEBUG(fmtstring, ...) fprintf(stderr, "[%s] " fmtstring, __file_logging_name, ##__VA_ARGS__)
#define LOG_DEBUG_UNPREFIXED(fmtstring, ...) fprintf(stderr, fmtstring, ##__VA_ARGS__)
#else
#define DEBUG_ASSERT(__cond) do {} while (0)
#define DEBUG_ASSERT_MSG(__cond, __msg) do {} while (0)
#define LOG_ERROR(fmtstring, ...) fprintf(stderr, "[%s] " fmtstring, __file_logging_name, ##__VA_ARGS__)
#define LOG_ERROR_UNPREFIXED(fmtstring, ...) fprintf(stderr, fmtstring, ##__VA_ARGS__)
#define LOG_DEBUG(fmtstring, ...) do {} while (0)
#define LOG_DEBUG_UNPREFIXED(fmtstring, ...) do {} while (0)
#endif

#define DEBUG_ASSERT_NOT_NULL(__var) DEBUG_ASSERT(__var != NULL)
#define DEBUG_ASSERT_EQUALS(__a, __b) DEBUG_ASSERT((__a) == (__b))
#define DEBUG_ASSERT_EGL_TRUE(__var) DEBUG_ASSERT((__var) == EGL_TRUE)

#if !(201112L <= __STDC_VERSION__ || (!defined __STRICT_ANSI__ && (__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR >= 6))))
#	error "Needs C11 or later or GCC (not in pedantic mode) 4.6.0 or later for compile time asserts."
#endif

#define COMPILE_ASSERT_MSG(expression, msg) _Static_assert(expression, msg)
#define COMPILE_ASSERT(expression) COMPILE_ASSERT_MSG(expression, "Expression evaluates to false")

#define UNIMPLEMENTED() assert(0 && "Unimplemented")