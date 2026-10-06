#pragma once

#include <string>

namespace WanaMemory
{

/* Writes memory, relationship scores, and WIT/WAY/WAI/WAMI for one pair into
   a temporary database, reopens it, and checks retrieval, prompt assembly,
   and a mock provider swap. Returns the number of failed checks. OutReport
   is a line-oriented log. */
int RunMemoryRoundtripScenario(std::string& OutReport);

} // namespace WanaMemory
