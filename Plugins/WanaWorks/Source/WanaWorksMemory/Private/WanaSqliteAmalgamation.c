#include "WanaSqliteConfig.h"

#if defined(__clang__)
#  pragma clang diagnostic ignored "-Weverything"
#elif defined(__GNUC__)
#  pragma GCC diagnostic ignored "-Wall"
#  pragma GCC diagnostic ignored "-Wextra"
#  pragma GCC diagnostic ignored "-Wpedantic"
#  pragma GCC diagnostic ignored "-Wunused-function"
#  pragma GCC diagnostic ignored "-Wunused-parameter"
#  pragma GCC diagnostic ignored "-Wcast-qual"
#endif

#if defined(_MSC_VER)
#  pragma warning(push, 0)
#endif

#include "sqlite3.c.inc"

#if defined(_MSC_VER)
#  pragma warning(pop)
#endif
