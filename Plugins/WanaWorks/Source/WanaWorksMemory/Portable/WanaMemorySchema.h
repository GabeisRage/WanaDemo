#pragma once

namespace WanaMemory
{

struct SchemaMigration
{
    int Version = 0;
    const char* Sql = "";
};

int LatestSchemaVersion();
const SchemaMigration* SchemaMigrations(int& OutCount);

} // namespace WanaMemory
