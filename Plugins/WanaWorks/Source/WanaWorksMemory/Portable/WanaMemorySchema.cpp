#include "WanaMemorySchema.h"

namespace WanaMemory
{
namespace
{

const char* kMigration1 = R"SQL(
CREATE TABLE schema_migrations (
    version INTEGER PRIMARY KEY,
    applied_at TEXT NOT NULL
);
CREATE TABLE conversation_turns (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    character_id TEXT NOT NULL,
    player_id TEXT NOT NULL,
    role TEXT NOT NULL,
    content TEXT NOT NULL,
    created_at TEXT NOT NULL,
    ordinal INTEGER NOT NULL
);
CREATE INDEX idx_turns_pair_ordinal ON conversation_turns(character_id, player_id, ordinal);
CREATE TABLE relationship_scores (
    character_id TEXT NOT NULL,
    player_id TEXT NOT NULL,
    trust REAL NOT NULL,
    affinity REAL NOT NULL,
    fear REAL NOT NULL,
    respect REAL NOT NULL,
    updated_at TEXT NOT NULL,
    PRIMARY KEY (character_id, player_id)
);
CREATE TABLE relationship_history (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    character_id TEXT NOT NULL,
    player_id TEXT NOT NULL,
    trust REAL NOT NULL,
    affinity REAL NOT NULL,
    fear REAL NOT NULL,
    respect REAL NOT NULL,
    delta_trust REAL NOT NULL,
    delta_affinity REAL NOT NULL,
    delta_fear REAL NOT NULL,
    delta_respect REAL NOT NULL,
    reason TEXT NOT NULL,
    source TEXT NOT NULL,
    created_at TEXT NOT NULL
);
CREATE INDEX idx_rel_hist_pair ON relationship_history(character_id, player_id, id);
CREATE TABLE identity_state (
    character_id TEXT NOT NULL,
    state_kind TEXT NOT NULL,
    schema_version INTEGER NOT NULL,
    json_blob TEXT NOT NULL,
    updated_at TEXT NOT NULL,
    PRIMARY KEY (character_id, state_kind)
);
CREATE TABLE salient_memories (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    character_id TEXT NOT NULL,
    player_id TEXT NOT NULL,
    content TEXT NOT NULL,
    keywords TEXT NOT NULL,
    importance REAL NOT NULL,
    embedding BLOB,
    embedding_dim INTEGER NOT NULL DEFAULT 0,
    created_at TEXT NOT NULL,
    last_used_at TEXT,
    source TEXT NOT NULL DEFAULT 'conversation'
);
CREATE INDEX idx_salient_pair ON salient_memories(character_id, player_id, importance);
)SQL";

const char* kMigration2 = R"SQL(
ALTER TABLE conversation_turns ADD COLUMN provider_id TEXT NOT NULL DEFAULT '';
)SQL";

const SchemaMigration kMigrations[] = {
    {1, kMigration1},
    {2, kMigration2}
};

} // namespace

int LatestSchemaVersion()
{
    return 2;
}

const SchemaMigration* SchemaMigrations(int& OutCount)
{
    OutCount = static_cast<int>(sizeof(kMigrations) / sizeof(kMigrations[0]));
    return kMigrations;
}

} // namespace WanaMemory
