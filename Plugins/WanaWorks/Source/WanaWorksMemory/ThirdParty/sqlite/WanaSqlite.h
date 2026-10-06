#pragma once

/* Include this wrapper instead of sqlite3.h so the amalgamation and the
   callers agree on SQLITE_API and the compile-time options. */
#include "WanaSqliteConfig.h"
#include "sqlite3.h"
