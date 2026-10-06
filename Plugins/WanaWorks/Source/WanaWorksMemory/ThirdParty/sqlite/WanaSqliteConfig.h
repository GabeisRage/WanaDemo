#ifndef WANA_SQLITE_CONFIG_H
#define WANA_SQLITE_CONFIG_H

/* Vendored SQLite is private to WanaWorksMemory. Hide the C API so it does not
   collide with the engine SQLiteCore plugin if that plugin is also loaded. */
#ifndef SQLITE_API
#  if defined(__GNUC__) || defined(__clang__)
#    define SQLITE_API __attribute__((visibility("hidden")))
#  else
#    define SQLITE_API
#  endif
#endif

#ifndef SQLITE_OMIT_LOAD_EXTENSION
#  define SQLITE_OMIT_LOAD_EXTENSION 1
#endif

#ifndef SQLITE_DQS
#  define SQLITE_DQS 0
#endif

#ifndef SQLITE_THREADSAFE
#  define SQLITE_THREADSAFE 1
#endif

#ifndef SQLITE_DEFAULT_MEMSTATUS
#  define SQLITE_DEFAULT_MEMSTATUS 0
#endif

#endif
