#include "WanaMemoryStore.h"

#include "WanaMemorySchema.h"
#include "WanaMemoryUtil.h"
#include "WanaSqlite.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <utility>

namespace WanaMemory
{
namespace
{

Status Exec(sqlite3* Db, const char* Sql)
{
    char* Error = nullptr;
    const int Rc = sqlite3_exec(Db, Sql, nullptr, nullptr, &Error);
    if (Rc != SQLITE_OK)
    {
        const std::string Message = Error ? Error : sqlite3_errmsg(Db);
        sqlite3_free(Error);
        return Status::Fail(Message);
    }
    return Status::Ok();
}

class SqlStmt
{
public:
    ~SqlStmt()
    {
        sqlite3_finalize(Stmt);
    }

    SqlStmt(const SqlStmt&) = delete;
    SqlStmt& operator=(const SqlStmt&) = delete;
    SqlStmt() = default;

    Status Prepare(sqlite3* InDb, const char* Sql)
    {
        Db = InDb;
        const int Rc = sqlite3_prepare_v2(Db, Sql, -1, &Stmt, nullptr);
        if (Rc != SQLITE_OK)
        {
            return Status::Fail(sqlite3_errmsg(Db));
        }
        return Status::Ok();
    }

    Status BindText(int Index, const std::string& Value)
    {
        const int Rc = sqlite3_bind_text(Stmt, Index, Value.c_str(), static_cast<int>(Value.size()), SQLITE_TRANSIENT);
        if (Rc != SQLITE_OK)
        {
            return Status::Fail(sqlite3_errmsg(Db));
        }
        return Status::Ok();
    }

    Status BindDouble(int Index, double Value)
    {
        const int Rc = sqlite3_bind_double(Stmt, Index, Value);
        if (Rc != SQLITE_OK)
        {
            return Status::Fail(sqlite3_errmsg(Db));
        }
        return Status::Ok();
    }

    Status BindInt(int Index, int Value)
    {
        const int Rc = sqlite3_bind_int(Stmt, Index, Value);
        if (Rc != SQLITE_OK)
        {
            return Status::Fail(sqlite3_errmsg(Db));
        }
        return Status::Ok();
    }

    Status BindInt64(int Index, int64_t Value)
    {
        const int Rc = sqlite3_bind_int64(Stmt, Index, static_cast<sqlite3_int64>(Value));
        if (Rc != SQLITE_OK)
        {
            return Status::Fail(sqlite3_errmsg(Db));
        }
        return Status::Ok();
    }

    Status BindBlob(int Index, const void* Data, int Bytes)
    {
        const int Rc = Data ? sqlite3_bind_blob(Stmt, Index, Data, Bytes, SQLITE_TRANSIENT) : sqlite3_bind_null(Stmt, Index);
        if (Rc != SQLITE_OK)
        {
            return Status::Fail(sqlite3_errmsg(Db));
        }
        return Status::Ok();
    }

    int Step(Status& OutStatus)
    {
        const int Rc = sqlite3_step(Stmt);
        if (Rc != SQLITE_ROW && Rc != SQLITE_DONE)
        {
            OutStatus = Status::Fail(sqlite3_errmsg(Db));
        }
        return Rc;
    }

    std::string ColumnText(int Index) const
    {
        const unsigned char* Text = sqlite3_column_text(Stmt, Index);
        if (!Text)
        {
            return std::string();
        }
        const int Bytes = sqlite3_column_bytes(Stmt, Index);
        return std::string(reinterpret_cast<const char*>(Text), static_cast<std::size_t>(Bytes < 0 ? 0 : Bytes));
    }

    double ColumnDouble(int Index) const
    {
        return sqlite3_column_double(Stmt, Index);
    }

    int ColumnInt(int Index) const
    {
        return sqlite3_column_int(Stmt, Index);
    }

    int64_t ColumnInt64(int Index) const
    {
        return static_cast<int64_t>(sqlite3_column_int64(Stmt, Index));
    }

    sqlite3_stmt* Raw() const
    {
        return Stmt;
    }

private:
    sqlite3* Db = nullptr;
    sqlite3_stmt* Stmt = nullptr;
};

struct Txn
{
    sqlite3* Db = nullptr;
    bool bActive = false;

    Status Begin()
    {
        const Status Result = Exec(Db, "BEGIN IMMEDIATE;");
        bActive = Result.bOk;
        return Result;
    }

    Status Commit()
    {
        const Status Result = Exec(Db, "COMMIT;");
        if (Result.bOk)
        {
            bActive = false;
        }
        return Result;
    }

    void Rollback()
    {
        if (bActive)
        {
            sqlite3_exec(Db, "ROLLBACK;", nullptr, nullptr, nullptr);
            bActive = false;
        }
    }

    ~Txn()
    {
        Rollback();
    }
};

void ReadEmbedding(sqlite3_stmt* Stmt, SalientMemory& Memory)
{
    Memory.EmbeddingDim = sqlite3_column_int(Stmt, 7);
    if (sqlite3_column_type(Stmt, 6) == SQLITE_NULL || Memory.EmbeddingDim <= 0)
    {
        Memory.Embedding.clear();
        Memory.EmbeddingDim = 0;
        return;
    }
    const int Bytes = sqlite3_column_bytes(Stmt, 6);
    const void* Blob = sqlite3_column_blob(Stmt, 6);
    const int Expected = Memory.EmbeddingDim * static_cast<int>(sizeof(float));
    if (!Blob || Bytes != Expected)
    {
        Memory.Embedding.clear();
        Memory.EmbeddingDim = 0;
        return;
    }
    const float* Floats = static_cast<const float*>(Blob);
    Memory.Embedding.assign(Floats, Floats + Memory.EmbeddingDim);
}

SalientMemory ReadMemoryRow(sqlite3_stmt* Stmt)
{
    SalientMemory Memory;
    Memory.Id = static_cast<int64_t>(sqlite3_column_int64(Stmt, 0));
    const unsigned char* Character = sqlite3_column_text(Stmt, 1);
    const unsigned char* Player = sqlite3_column_text(Stmt, 2);
    const unsigned char* Content = sqlite3_column_text(Stmt, 3);
    const unsigned char* Keywords = sqlite3_column_text(Stmt, 4);
    Memory.CharacterId = Character ? reinterpret_cast<const char*>(Character) : "";
    Memory.PlayerId = Player ? reinterpret_cast<const char*>(Player) : "";
    Memory.Content = Content ? reinterpret_cast<const char*>(Content) : "";
    Memory.Keywords = Keywords ? reinterpret_cast<const char*>(Keywords) : "";
    Memory.Importance = sqlite3_column_double(Stmt, 5);
    ReadEmbedding(Stmt, Memory);
    const unsigned char* Created = sqlite3_column_text(Stmt, 8);
    const unsigned char* Used = sqlite3_column_text(Stmt, 9);
    const unsigned char* Source = sqlite3_column_text(Stmt, 10);
    Memory.CreatedAt = Created ? reinterpret_cast<const char*>(Created) : "";
    Memory.LastUsedAt = Used ? reinterpret_cast<const char*>(Used) : "";
    Memory.Source = Source ? reinterpret_cast<const char*>(Source) : "";
    return Memory;
}

void AppendUnique(std::vector<SalientMemory>& Out, SalientMemory Memory)
{
    for (const SalientMemory& Existing : Out)
    {
        if (Existing.Id == Memory.Id)
        {
            return;
        }
    }
    Out.push_back(std::move(Memory));
}

Status CollectMemories(sqlite3* Db, const char* Sql, const std::string& CharacterId, const std::string& PlayerId, int Limit, std::vector<SalientMemory>& Out)
{
    SqlStmt Stmt;
    Status Prepared = Stmt.Prepare(Db, Sql);
    if (!Prepared.bOk)
    {
        return Prepared;
    }
    if (Status Bound = Stmt.BindText(1, CharacterId); !Bound.bOk) return Bound;
    if (Status Bound = Stmt.BindText(2, PlayerId); !Bound.bOk) return Bound;
    if (Status Bound = Stmt.BindInt(3, Limit); !Bound.bOk) return Bound;
    Status StepStatus = Status::Ok();
    while (true)
    {
        const int Rc = Stmt.Step(StepStatus);
        if (Rc == SQLITE_DONE)
        {
            return Status::Ok();
        }
        if (Rc != SQLITE_ROW)
        {
            return StepStatus;
        }
        AppendUnique(Out, ReadMemoryRow(Stmt.Raw()));
    }
}

} // namespace

struct MemoryStore::Impl
{
    sqlite3* Db = nullptr;
    std::string Path;
    int SchemaVersion = -1;
    mutable std::mutex Mutex;

    Status ApplyMigrations()
    {
        int Version = 0;
        {
            SqlStmt VersionStmt;
            Status Prepared = VersionStmt.Prepare(Db, "PRAGMA user_version;");
            if (!Prepared.bOk)
            {
                return Prepared;
            }
            Status StepStatus = Status::Ok();
            const int Rc = VersionStmt.Step(StepStatus);
            if (Rc != SQLITE_ROW)
            {
                return StepStatus.bOk ? Status::Fail("could not read schema version") : StepStatus;
            }
            Version = VersionStmt.ColumnInt(0);
        }
        if (Version > LatestSchemaVersion())
        {
            return Status::Fail("database schema is newer than this WanaWorksMemory build");
        }

        int Count = 0;
        const SchemaMigration* Migrations = SchemaMigrations(Count);
        for (int Index = 0; Index < Count; ++Index)
        {
            if (Migrations[Index].Version <= Version)
            {
                continue;
            }
            Txn Transaction;
            Transaction.Db = Db;
            if (Status Began = Transaction.Begin(); !Began.bOk)
            {
                return Began;
            }
            if (Status Applied = Exec(Db, Migrations[Index].Sql); !Applied.bOk)
            {
                Transaction.Rollback();
                return Applied;
            }
            SqlStmt Insert;
            Status Prepared = Insert.Prepare(Db, "INSERT INTO schema_migrations(version, applied_at) VALUES(?, ?);");
            if (!Prepared.bOk)
            {
                Transaction.Rollback();
                return Prepared;
            }
            if (Status Bound = Insert.BindInt(1, Migrations[Index].Version); !Bound.bOk)
            {
                Transaction.Rollback();
                return Bound;
            }
            if (Status Bound = Insert.BindText(2, UtcNow()); !Bound.bOk)
            {
                Transaction.Rollback();
                return Bound;
            }
            Status StepStatus = Status::Ok();
            const int InsertRc = Insert.Step(StepStatus);
            if (InsertRc != SQLITE_DONE)
            {
                Transaction.Rollback();
                return StepStatus.bOk ? Status::Fail("could not record schema migration") : StepStatus;
            }
            const std::string Pragma = "PRAGMA user_version = " + std::to_string(Migrations[Index].Version) + ";";
            if (Status Stamped = Exec(Db, Pragma.c_str()); !Stamped.bOk)
            {
                Transaction.Rollback();
                return Stamped;
            }
            if (Status Committed = Transaction.Commit(); !Committed.bOk)
            {
                return Committed;
            }
            Version = Migrations[Index].Version;
        }
        SchemaVersion = Version;
        return Status::Ok();
    }

    Status LoadRelationship(const std::string& CharacterId, const std::string& PlayerId, RelationshipScores& OutScores, bool& bFound)
    {
        SqlStmt Stmt;
        Status Prepared = Stmt.Prepare(Db, "SELECT trust, affinity, fear, respect FROM relationship_scores WHERE character_id = ? AND player_id = ?;");
        if (!Prepared.bOk)
        {
            return Prepared;
        }
        if (Status Bound = Stmt.BindText(1, CharacterId); !Bound.bOk) return Bound;
        if (Status Bound = Stmt.BindText(2, PlayerId); !Bound.bOk) return Bound;
        Status StepStatus = Status::Ok();
        const int Rc = Stmt.Step(StepStatus);
        if (Rc == SQLITE_DONE)
        {
            OutScores = DefaultRelationship();
            bFound = false;
            return Status::Ok();
        }
        if (Rc != SQLITE_ROW)
        {
            return StepStatus;
        }
        OutScores.Trust = Stmt.ColumnDouble(0);
        OutScores.Affinity = Stmt.ColumnDouble(1);
        OutScores.Fear = Stmt.ColumnDouble(2);
        OutScores.Respect = Stmt.ColumnDouble(3);
        bFound = true;
        return Status::Ok();
    }

    Status WriteRelationship(const std::string& CharacterId, const std::string& PlayerId, const RelationshipScores& Before, const RelationshipScores& After, const RelationshipDelta& Applied, bool bFound, bool& bChanged)
    {
        const double DeltaTrust = After.Trust - Before.Trust;
        const double DeltaAffinity = After.Affinity - Before.Affinity;
        const double DeltaFear = After.Fear - Before.Fear;
        const double DeltaRespect = After.Respect - Before.Respect;
        bChanged = !NearZero(DeltaTrust) || !NearZero(DeltaAffinity) || !NearZero(DeltaFear) || !NearZero(DeltaRespect);
        if (!bChanged && bFound)
        {
            return Status::Ok();
        }
        if (!bChanged && !bFound)
        {
            return Status::Ok();
        }

        Txn Transaction;
        Transaction.Db = Db;
        if (Status Began = Transaction.Begin(); !Began.bOk)
        {
            return Began;
        }
        const std::string Now = UtcNow();
        SqlStmt Upsert;
        Status Prepared = Upsert.Prepare(Db,
            "INSERT INTO relationship_scores(character_id, player_id, trust, affinity, fear, respect, updated_at) "
            "VALUES(?, ?, ?, ?, ?, ?, ?) "
            "ON CONFLICT(character_id, player_id) DO UPDATE SET "
            "trust = excluded.trust, affinity = excluded.affinity, fear = excluded.fear, respect = excluded.respect, updated_at = excluded.updated_at;");
        if (!Prepared.bOk)
        {
            return Prepared;
        }
        if (Status Bound = Upsert.BindText(1, CharacterId); !Bound.bOk) return Bound;
        if (Status Bound = Upsert.BindText(2, PlayerId); !Bound.bOk) return Bound;
        if (Status Bound = Upsert.BindDouble(3, After.Trust); !Bound.bOk) return Bound;
        if (Status Bound = Upsert.BindDouble(4, After.Affinity); !Bound.bOk) return Bound;
        if (Status Bound = Upsert.BindDouble(5, After.Fear); !Bound.bOk) return Bound;
        if (Status Bound = Upsert.BindDouble(6, After.Respect); !Bound.bOk) return Bound;
        if (Status Bound = Upsert.BindText(7, Now); !Bound.bOk) return Bound;
        Status StepStatus = Status::Ok();
        if (Upsert.Step(StepStatus) != SQLITE_DONE)
        {
            return StepStatus.bOk ? Status::Fail("could not store relationship scores") : StepStatus;
        }

        SqlStmt History;
        Prepared = History.Prepare(Db,
            "INSERT INTO relationship_history(character_id, player_id, trust, affinity, fear, respect, delta_trust, delta_affinity, delta_fear, delta_respect, reason, source, created_at) "
            "VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);");
        if (!Prepared.bOk)
        {
            return Prepared;
        }
        if (Status Bound = History.BindText(1, CharacterId); !Bound.bOk) return Bound;
        if (Status Bound = History.BindText(2, PlayerId); !Bound.bOk) return Bound;
        if (Status Bound = History.BindDouble(3, After.Trust); !Bound.bOk) return Bound;
        if (Status Bound = History.BindDouble(4, After.Affinity); !Bound.bOk) return Bound;
        if (Status Bound = History.BindDouble(5, After.Fear); !Bound.bOk) return Bound;
        if (Status Bound = History.BindDouble(6, After.Respect); !Bound.bOk) return Bound;
        if (Status Bound = History.BindDouble(7, DeltaTrust); !Bound.bOk) return Bound;
        if (Status Bound = History.BindDouble(8, DeltaAffinity); !Bound.bOk) return Bound;
        if (Status Bound = History.BindDouble(9, DeltaFear); !Bound.bOk) return Bound;
        if (Status Bound = History.BindDouble(10, DeltaRespect); !Bound.bOk) return Bound;
        if (Status Bound = History.BindText(11, Applied.Reason); !Bound.bOk) return Bound;
        if (Status Bound = History.BindText(12, Applied.Source); !Bound.bOk) return Bound;
        if (Status Bound = History.BindText(13, Now); !Bound.bOk) return Bound;
        StepStatus = Status::Ok();
        if (History.Step(StepStatus) != SQLITE_DONE)
        {
            return StepStatus.bOk ? Status::Fail("could not store relationship history") : StepStatus;
        }
        return Transaction.Commit();
    }
};

MemoryStore::MemoryStore()
    : ImplPtr(new Impl())
{
}

MemoryStore::~MemoryStore()
{
    Close();
}

Status MemoryStore::Open(const std::string& InPath)
{
    std::lock_guard<std::mutex> Lock(ImplPtr->Mutex);
    if (ImplPtr->Db)
    {
        return Status::Fail("memory store is already open");
    }
    if (InPath.empty())
    {
        return Status::Fail("database path is required");
    }
    if (InPath != ":memory:" && !EnsureParentDirectory(InPath))
    {
        return Status::Fail("could not create the directory for the memory database");
    }

    sqlite3* Db = nullptr;
    const int Rc = sqlite3_open_v2(InPath.c_str(), &Db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, nullptr);
    if (Rc != SQLITE_OK)
    {
        const std::string Message = Db ? sqlite3_errmsg(Db) : "could not open sqlite";
        if (Db)
        {
            sqlite3_close(Db);
        }
        return Status::Fail(Message);
    }
    sqlite3_busy_timeout(Db, 3000);
    ImplPtr->Db = Db;
    ImplPtr->Path = InPath;
    if (Status Migrated = ImplPtr->ApplyMigrations(); !Migrated.bOk)
    {
        sqlite3_close(ImplPtr->Db);
        ImplPtr->Db = nullptr;
        ImplPtr->Path.clear();
        ImplPtr->SchemaVersion = -1;
        return Migrated;
    }
    Exec(ImplPtr->Db, "PRAGMA journal_mode=WAL;");
    Exec(ImplPtr->Db, "PRAGMA synchronous=NORMAL;");
    return Status::Ok();
}

Status MemoryStore::OpenView(const std::string& InPath)
{
    std::lock_guard<std::mutex> Lock(ImplPtr->Mutex);
    if (ImplPtr->Db)
    {
        return Status::Fail("memory store is already open");
    }
    if (InPath.empty() || InPath == ":memory:")
    {
        return Status::Fail("a database file is required");
    }

    sqlite3* Db = nullptr;
    const int Rc = sqlite3_open_v2(InPath.c_str(), &Db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_FULLMUTEX, nullptr);
    if (Rc != SQLITE_OK)
    {
        const std::string Message = Db ? sqlite3_errmsg(Db) : "could not open sqlite";
        if (Db)
        {
            sqlite3_close(Db);
        }
        return Status::Fail(Message);
    }
    sqlite3_busy_timeout(Db, 3000);
    if (Status Locked = Exec(Db, "PRAGMA query_only=ON;"); !Locked.bOk)
    {
        sqlite3_close(Db);
        return Locked;
    }
    ImplPtr->Db = Db;
    ImplPtr->Path = InPath;
    SqlStmt VersionStmt;
    Status Prepared = VersionStmt.Prepare(Db, "PRAGMA user_version;");
    if (Prepared.bOk)
    {
        Status StepStatus = Status::Ok();
        if (VersionStmt.Step(StepStatus) == SQLITE_ROW)
        {
            ImplPtr->SchemaVersion = VersionStmt.ColumnInt(0);
        }
    }
    return Status::Ok();
}

void MemoryStore::Close()
{
    if (!ImplPtr)
    {
        return;
    }
    std::lock_guard<std::mutex> Lock(ImplPtr->Mutex);
    if (ImplPtr->Db)
    {
        sqlite3_close(ImplPtr->Db);
        ImplPtr->Db = nullptr;
    }
    ImplPtr->Path.clear();
    ImplPtr->SchemaVersion = -1;
}

bool MemoryStore::IsOpen() const
{
    std::lock_guard<std::mutex> Lock(ImplPtr->Mutex);
    return ImplPtr->Db != nullptr;
}

int MemoryStore::SchemaVersion() const
{
    std::lock_guard<std::mutex> Lock(ImplPtr->Mutex);
    return ImplPtr->SchemaVersion;
}

std::string MemoryStore::Path() const
{
    std::lock_guard<std::mutex> Lock(ImplPtr->Mutex);
    return ImplPtr->Path;
}

Status MemoryStore::AddTurn(const Turn& InTurn, int64_t& OutId)
{
    std::lock_guard<std::mutex> Lock(ImplPtr->Mutex);
    if (!ImplPtr->Db)
    {
        return Status::Fail("memory store is not open");
    }
    if (Status Pair = ValidatePair(InTurn.CharacterId, InTurn.PlayerId); !Pair.bOk)
    {
        return Pair;
    }
    if (!IsAllowedTurnRole(InTurn.Role))
    {
        return Status::Fail("turn role must be user, assistant, system, or tool");
    }
    if (InTurn.Content.empty() || InTurn.Content.size() > static_cast<std::size_t>(kMaxContentChars))
    {
        return Status::Fail("turn content must be between 1 and 100000 characters");
    }

    Txn Transaction;
    Transaction.Db = ImplPtr->Db;
    if (Status Began = Transaction.Begin(); !Began.bOk)
    {
        return Began;
    }

    SqlStmt Next;
    Status Prepared = Next.Prepare(ImplPtr->Db, "SELECT COALESCE(MAX(ordinal), 0) + 1 FROM conversation_turns WHERE character_id = ? AND player_id = ?;");
    if (!Prepared.bOk)
    {
        return Prepared;
    }
    if (Status Bound = Next.BindText(1, InTurn.CharacterId); !Bound.bOk) return Bound;
    if (Status Bound = Next.BindText(2, InTurn.PlayerId); !Bound.bOk) return Bound;
    Status StepStatus = Status::Ok();
    if (Next.Step(StepStatus) != SQLITE_ROW)
    {
        return StepStatus.bOk ? Status::Fail("could not allocate a turn ordinal") : StepStatus;
    }
    const int64_t Ordinal = Next.ColumnInt64(0);

    SqlStmt Insert;
    Prepared = Insert.Prepare(ImplPtr->Db,
        "INSERT INTO conversation_turns(character_id, player_id, role, content, created_at, ordinal, provider_id) "
        "VALUES(?, ?, ?, ?, ?, ?, ?);");
    if (!Prepared.bOk)
    {
        return Prepared;
    }
    const std::string Created = InTurn.CreatedAt.empty() ? UtcNow() : InTurn.CreatedAt;
    if (Status Bound = Insert.BindText(1, InTurn.CharacterId); !Bound.bOk) return Bound;
    if (Status Bound = Insert.BindText(2, InTurn.PlayerId); !Bound.bOk) return Bound;
    if (Status Bound = Insert.BindText(3, InTurn.Role); !Bound.bOk) return Bound;
    if (Status Bound = Insert.BindText(4, InTurn.Content); !Bound.bOk) return Bound;
    if (Status Bound = Insert.BindText(5, Created); !Bound.bOk) return Bound;
    if (Status Bound = Insert.BindInt64(6, Ordinal); !Bound.bOk) return Bound;
    if (Status Bound = Insert.BindText(7, InTurn.ProviderId); !Bound.bOk) return Bound;
    StepStatus = Status::Ok();
    if (Insert.Step(StepStatus) != SQLITE_DONE)
    {
        return StepStatus.bOk ? Status::Fail("could not store the conversation turn") : StepStatus;
    }
    OutId = static_cast<int64_t>(sqlite3_last_insert_rowid(ImplPtr->Db));
    return Transaction.Commit();
}

Status MemoryStore::GetRecentTurns(const std::string& CharacterId, const std::string& PlayerId, int Limit, std::vector<Turn>& OutTurns)
{
    std::lock_guard<std::mutex> Lock(ImplPtr->Mutex);
    OutTurns.clear();
    if (!ImplPtr->Db)
    {
        return Status::Fail("memory store is not open");
    }
    if (Status Pair = ValidatePair(CharacterId, PlayerId); !Pair.bOk)
    {
        return Pair;
    }
    if (Limit < 1)
    {
        Limit = 1;
    }
    if (Limit > 200)
    {
        Limit = 200;
    }
    SqlStmt Stmt;
    Status Prepared = Stmt.Prepare(ImplPtr->Db,
        "SELECT id, character_id, player_id, role, content, created_at, ordinal, provider_id "
        "FROM conversation_turns WHERE character_id = ? AND player_id = ? "
        "ORDER BY ordinal DESC LIMIT ?;");
    if (!Prepared.bOk)
    {
        return Prepared;
    }
    if (Status Bound = Stmt.BindText(1, CharacterId); !Bound.bOk) return Bound;
    if (Status Bound = Stmt.BindText(2, PlayerId); !Bound.bOk) return Bound;
    if (Status Bound = Stmt.BindInt(3, Limit); !Bound.bOk) return Bound;
    Status StepStatus = Status::Ok();
    while (true)
    {
        const int Rc = Stmt.Step(StepStatus);
        if (Rc == SQLITE_DONE)
        {
            break;
        }
        if (Rc != SQLITE_ROW)
        {
            return StepStatus;
        }
        Turn Row;
        Row.Id = Stmt.ColumnInt64(0);
        Row.CharacterId = Stmt.ColumnText(1);
        Row.PlayerId = Stmt.ColumnText(2);
        Row.Role = Stmt.ColumnText(3);
        Row.Content = Stmt.ColumnText(4);
        Row.CreatedAt = Stmt.ColumnText(5);
        Row.Ordinal = Stmt.ColumnInt64(6);
        Row.ProviderId = Stmt.ColumnText(7);
        OutTurns.push_back(std::move(Row));
    }
    std::reverse(OutTurns.begin(), OutTurns.end());
    return Status::Ok();
}

Status MemoryStore::CountTurns(const std::string& CharacterId, const std::string& PlayerId, int& OutCount)
{
    std::lock_guard<std::mutex> Lock(ImplPtr->Mutex);
    OutCount = 0;
    if (!ImplPtr->Db)
    {
        return Status::Fail("memory store is not open");
    }
    if (Status Pair = ValidatePair(CharacterId, PlayerId); !Pair.bOk)
    {
        return Pair;
    }
    SqlStmt Stmt;
    Status Prepared = Stmt.Prepare(ImplPtr->Db, "SELECT COUNT(*) FROM conversation_turns WHERE character_id = ? AND player_id = ?;");
    if (!Prepared.bOk)
    {
        return Prepared;
    }
    if (Status Bound = Stmt.BindText(1, CharacterId); !Bound.bOk) return Bound;
    if (Status Bound = Stmt.BindText(2, PlayerId); !Bound.bOk) return Bound;
    Status StepStatus = Status::Ok();
    if (Stmt.Step(StepStatus) != SQLITE_ROW)
    {
        return StepStatus.bOk ? Status::Fail("could not count turns") : StepStatus;
    }
    OutCount = Stmt.ColumnInt(0);
    return Status::Ok();
}

Status MemoryStore::GetRelationship(const std::string& CharacterId, const std::string& PlayerId, RelationshipScores& OutScores, bool& bFound)
{
    std::lock_guard<std::mutex> Lock(ImplPtr->Mutex);
    bFound = false;
    OutScores = DefaultRelationship();
    if (!ImplPtr->Db)
    {
        return Status::Fail("memory store is not open");
    }
    if (Status Pair = ValidatePair(CharacterId, PlayerId); !Pair.bOk)
    {
        return Pair;
    }
    return ImplPtr->LoadRelationship(CharacterId, PlayerId, OutScores, bFound);
}

Status MemoryStore::ApplyRelationshipDelta(const RelationshipDelta& Delta, RelationshipScores& OutScores, bool& bChanged)
{
    std::lock_guard<std::mutex> Lock(ImplPtr->Mutex);
    bChanged = false;
    OutScores = DefaultRelationship();
    if (!ImplPtr->Db)
    {
        return Status::Fail("memory store is not open");
    }
    if (Status Pair = ValidatePair(Delta.CharacterId, Delta.PlayerId); !Pair.bOk)
    {
        return Pair;
    }
    bool bFound = false;
    RelationshipScores Before;
    if (Status Loaded = ImplPtr->LoadRelationship(Delta.CharacterId, Delta.PlayerId, Before, bFound); !Loaded.bOk)
    {
        return Loaded;
    }
    const double Cap = Delta.MaxAbsPerAxis > 0.0 ? Delta.MaxAbsPerAxis : kModelDeltaCap;
    RelationshipScores After = Before;
    After.Trust = Clamp01(Before.Trust + ClampAbs(Delta.Trust, Cap));
    After.Affinity = Clamp01(Before.Affinity + ClampAbs(Delta.Affinity, Cap));
    After.Fear = Clamp01(Before.Fear + ClampAbs(Delta.Fear, Cap));
    After.Respect = Clamp01(Before.Respect + ClampAbs(Delta.Respect, Cap));
    if (Status Written = ImplPtr->WriteRelationship(Delta.CharacterId, Delta.PlayerId, Before, After, Delta, bFound, bChanged); !Written.bOk)
    {
        return Written;
    }
    OutScores = bChanged ? After : Before;
    return Status::Ok();
}

Status MemoryStore::SetRelationship(const std::string& CharacterId, const std::string& PlayerId, const RelationshipScores& Scores, const std::string& Reason, const std::string& Source, bool& bChanged)
{
    RelationshipDelta Delta;
    Delta.CharacterId = CharacterId;
    Delta.PlayerId = PlayerId;
    Delta.Reason = Reason;
    Delta.Source = Source;
    Delta.MaxAbsPerAxis = 1.0;
    std::lock_guard<std::mutex> Lock(ImplPtr->Mutex);
    bChanged = false;
    if (!ImplPtr->Db)
    {
        return Status::Fail("memory store is not open");
    }
    if (Status Pair = ValidatePair(CharacterId, PlayerId); !Pair.bOk)
    {
        return Pair;
    }
    bool bFound = false;
    RelationshipScores Before;
    if (Status Loaded = ImplPtr->LoadRelationship(CharacterId, PlayerId, Before, bFound); !Loaded.bOk)
    {
        return Loaded;
    }
    RelationshipScores After;
    After.Trust = Clamp01(Scores.Trust);
    After.Affinity = Clamp01(Scores.Affinity);
    After.Fear = Clamp01(Scores.Fear);
    After.Respect = Clamp01(Scores.Respect);
    return ImplPtr->WriteRelationship(CharacterId, PlayerId, Before, After, Delta, bFound, bChanged);
}

Status MemoryStore::GetRelationshipHistory(const std::string& CharacterId, const std::string& PlayerId, int Limit, std::vector<RelationshipEvent>& OutEvents)
{
    std::lock_guard<std::mutex> Lock(ImplPtr->Mutex);
    OutEvents.clear();
    if (!ImplPtr->Db)
    {
        return Status::Fail("memory store is not open");
    }
    if (Status Pair = ValidatePair(CharacterId, PlayerId); !Pair.bOk)
    {
        return Pair;
    }
    if (Limit < 1)
    {
        Limit = 1;
    }
    if (Limit > 200)
    {
        Limit = 200;
    }
    SqlStmt Stmt;
    Status Prepared = Stmt.Prepare(ImplPtr->Db,
        "SELECT id, trust, affinity, fear, respect, delta_trust, delta_affinity, delta_fear, delta_respect, reason, source, created_at "
        "FROM relationship_history WHERE character_id = ? AND player_id = ? ORDER BY id DESC LIMIT ?;");
    if (!Prepared.bOk)
    {
        return Prepared;
    }
    if (Status Bound = Stmt.BindText(1, CharacterId); !Bound.bOk) return Bound;
    if (Status Bound = Stmt.BindText(2, PlayerId); !Bound.bOk) return Bound;
    if (Status Bound = Stmt.BindInt(3, Limit); !Bound.bOk) return Bound;
    Status StepStatus = Status::Ok();
    while (true)
    {
        const int Rc = Stmt.Step(StepStatus);
        if (Rc == SQLITE_DONE)
        {
            return Status::Ok();
        }
        if (Rc != SQLITE_ROW)
        {
            return StepStatus;
        }
        RelationshipEvent Event;
        Event.Id = Stmt.ColumnInt64(0);
        Event.CharacterId = CharacterId;
        Event.PlayerId = PlayerId;
        Event.Scores.Trust = Stmt.ColumnDouble(1);
        Event.Scores.Affinity = Stmt.ColumnDouble(2);
        Event.Scores.Fear = Stmt.ColumnDouble(3);
        Event.Scores.Respect = Stmt.ColumnDouble(4);
        Event.Delta.Trust = Stmt.ColumnDouble(5);
        Event.Delta.Affinity = Stmt.ColumnDouble(6);
        Event.Delta.Fear = Stmt.ColumnDouble(7);
        Event.Delta.Respect = Stmt.ColumnDouble(8);
        Event.Delta.Reason = Stmt.ColumnText(9);
        Event.Delta.Source = Stmt.ColumnText(10);
        Event.CreatedAt = Stmt.ColumnText(11);
        Event.Delta.CharacterId = CharacterId;
        Event.Delta.PlayerId = PlayerId;
        OutEvents.push_back(std::move(Event));
    }
}

Status MemoryStore::SetIdentity(const IdentityState& State)
{
    std::lock_guard<std::mutex> Lock(ImplPtr->Mutex);
    if (!ImplPtr->Db)
    {
        return Status::Fail("memory store is not open");
    }
    if (Trim(State.CharacterId).empty() || State.CharacterId.size() > static_cast<std::size_t>(kMaxIdChars))
    {
        return Status::Fail("character id is required");
    }
    Status KindStatus;
    const std::string Kind = NormalizeStateKind(State.StateKind, KindStatus);
    if (!KindStatus.bOk)
    {
        return KindStatus;
    }
    if (State.SchemaVersion < 1)
    {
        return Status::Fail("identity schema version must be 1 or greater");
    }
    if (State.JsonBlob.empty() || State.JsonBlob.size() > static_cast<std::size_t>(kMaxIdentityChars))
    {
        return Status::Fail("identity JSON must be between 1 and 256000 characters");
    }
    SqlStmt Stmt;
    Status Prepared = Stmt.Prepare(ImplPtr->Db,
        "INSERT INTO identity_state(character_id, state_kind, schema_version, json_blob, updated_at) "
        "VALUES(?, ?, ?, ?, ?) "
        "ON CONFLICT(character_id, state_kind) DO UPDATE SET "
        "schema_version = excluded.schema_version, json_blob = excluded.json_blob, updated_at = excluded.updated_at;");
    if (!Prepared.bOk)
    {
        return Prepared;
    }
    if (Status Bound = Stmt.BindText(1, State.CharacterId); !Bound.bOk) return Bound;
    if (Status Bound = Stmt.BindText(2, Kind); !Bound.bOk) return Bound;
    if (Status Bound = Stmt.BindInt(3, State.SchemaVersion); !Bound.bOk) return Bound;
    if (Status Bound = Stmt.BindText(4, State.JsonBlob); !Bound.bOk) return Bound;
    if (Status Bound = Stmt.BindText(5, UtcNow()); !Bound.bOk) return Bound;
    Status StepStatus = Status::Ok();
    if (Stmt.Step(StepStatus) != SQLITE_DONE)
    {
        return StepStatus.bOk ? Status::Fail("could not store identity state") : StepStatus;
    }
    return Status::Ok();
}

Status MemoryStore::GetIdentity(const std::string& CharacterId, const std::string& StateKind, IdentityState& OutState, bool& bFound)
{
    std::lock_guard<std::mutex> Lock(ImplPtr->Mutex);
    bFound = false;
    OutState = IdentityState();
    if (!ImplPtr->Db)
    {
        return Status::Fail("memory store is not open");
    }
    if (Trim(CharacterId).empty())
    {
        return Status::Fail("character id is required");
    }
    Status KindStatus;
    const std::string Kind = NormalizeStateKind(StateKind, KindStatus);
    if (!KindStatus.bOk)
    {
        return KindStatus;
    }
    SqlStmt Stmt;
    Status Prepared = Stmt.Prepare(ImplPtr->Db,
        "SELECT schema_version, json_blob, updated_at FROM identity_state WHERE character_id = ? AND state_kind = ?;");
    if (!Prepared.bOk)
    {
        return Prepared;
    }
    if (Status Bound = Stmt.BindText(1, CharacterId); !Bound.bOk) return Bound;
    if (Status Bound = Stmt.BindText(2, Kind); !Bound.bOk) return Bound;
    Status StepStatus = Status::Ok();
    const int Rc = Stmt.Step(StepStatus);
    if (Rc == SQLITE_DONE)
    {
        return Status::Ok();
    }
    if (Rc != SQLITE_ROW)
    {
        return StepStatus;
    }
    OutState.CharacterId = CharacterId;
    OutState.StateKind = Kind;
    OutState.SchemaVersion = Stmt.ColumnInt(0);
    OutState.JsonBlob = Stmt.ColumnText(1);
    OutState.UpdatedAt = Stmt.ColumnText(2);
    bFound = true;
    return Status::Ok();
}

Status MemoryStore::ListIdentity(const std::string& CharacterId, std::vector<IdentityState>& OutStates)
{
    std::lock_guard<std::mutex> Lock(ImplPtr->Mutex);
    OutStates.clear();
    if (!ImplPtr->Db)
    {
        return Status::Fail("memory store is not open");
    }
    if (Trim(CharacterId).empty())
    {
        return Status::Fail("character id is required");
    }
    SqlStmt Stmt;
    Status Prepared = Stmt.Prepare(ImplPtr->Db,
        "SELECT state_kind, schema_version, json_blob, updated_at FROM identity_state WHERE character_id = ? ORDER BY state_kind;");
    if (!Prepared.bOk)
    {
        return Prepared;
    }
    if (Status Bound = Stmt.BindText(1, CharacterId); !Bound.bOk) return Bound;
    Status StepStatus = Status::Ok();
    while (true)
    {
        const int Rc = Stmt.Step(StepStatus);
        if (Rc == SQLITE_DONE)
        {
            return Status::Ok();
        }
        if (Rc != SQLITE_ROW)
        {
            return StepStatus;
        }
        IdentityState State;
        State.CharacterId = CharacterId;
        State.StateKind = Stmt.ColumnText(0);
        State.SchemaVersion = Stmt.ColumnInt(1);
        State.JsonBlob = Stmt.ColumnText(2);
        State.UpdatedAt = Stmt.ColumnText(3);
        OutStates.push_back(std::move(State));
    }
}

Status MemoryStore::AddMemory(const SalientMemory& InMemory, int64_t& OutId)
{
    std::lock_guard<std::mutex> Lock(ImplPtr->Mutex);
    if (!ImplPtr->Db)
    {
        return Status::Fail("memory store is not open");
    }
    if (Status Pair = ValidatePair(InMemory.CharacterId, InMemory.PlayerId); !Pair.bOk)
    {
        return Pair;
    }
    if (InMemory.Content.empty() || InMemory.Content.size() > static_cast<std::size_t>(kMaxContentChars))
    {
        return Status::Fail("memory content must be between 1 and 100000 characters");
    }
    const std::string Keywords = InMemory.Keywords.empty() ? JoinTokens(Tokenize(InMemory.Content)) : InMemory.Keywords;
    const double Importance = Clamp01(InMemory.Importance);
    const std::string Source = InMemory.Source.empty() ? std::string("conversation") : InMemory.Source;
    const int Dim = static_cast<int>(InMemory.Embedding.size());
    SqlStmt Stmt;
    Status Prepared = Stmt.Prepare(ImplPtr->Db,
        "INSERT INTO salient_memories(character_id, player_id, content, keywords, importance, embedding, embedding_dim, created_at, last_used_at, source) "
        "VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?);");
    if (!Prepared.bOk)
    {
        return Prepared;
    }
    if (Status Bound = Stmt.BindText(1, InMemory.CharacterId); !Bound.bOk) return Bound;
    if (Status Bound = Stmt.BindText(2, InMemory.PlayerId); !Bound.bOk) return Bound;
    if (Status Bound = Stmt.BindText(3, InMemory.Content); !Bound.bOk) return Bound;
    if (Status Bound = Stmt.BindText(4, Keywords); !Bound.bOk) return Bound;
    if (Status Bound = Stmt.BindDouble(5, Importance); !Bound.bOk) return Bound;
    if (Dim == 0)
    {
        if (Status Bound = Stmt.BindBlob(6, nullptr, 0); !Bound.bOk) return Bound;
    }
    else
    {
        if (Status Bound = Stmt.BindBlob(6, InMemory.Embedding.data(), Dim * static_cast<int>(sizeof(float))); !Bound.bOk) return Bound;
    }
    if (Status Bound = Stmt.BindInt(7, Dim); !Bound.bOk) return Bound;
    if (Status Bound = Stmt.BindText(8, InMemory.CreatedAt.empty() ? UtcNow() : InMemory.CreatedAt); !Bound.bOk) return Bound;
    if (InMemory.LastUsedAt.empty())
    {
        sqlite3_bind_null(Stmt.Raw(), 9);
    }
    else if (Status Bound = Stmt.BindText(9, InMemory.LastUsedAt); !Bound.bOk)
    {
        return Bound;
    }
    if (Status Bound = Stmt.BindText(10, Source); !Bound.bOk) return Bound;
    Status StepStatus = Status::Ok();
    if (Stmt.Step(StepStatus) != SQLITE_DONE)
    {
        return StepStatus.bOk ? Status::Fail("could not store salient memory") : StepStatus;
    }
    OutId = static_cast<int64_t>(sqlite3_last_insert_rowid(ImplPtr->Db));
    return Status::Ok();
}

Status MemoryStore::ListMemories(const std::string& CharacterId, const std::string& PlayerId, int Limit, std::vector<SalientMemory>& OutMemories)
{
    std::lock_guard<std::mutex> Lock(ImplPtr->Mutex);
    OutMemories.clear();
    if (!ImplPtr->Db)
    {
        return Status::Fail("memory store is not open");
    }
    if (Status Pair = ValidatePair(CharacterId, PlayerId); !Pair.bOk)
    {
        return Pair;
    }
    if (Limit < 1)
    {
        Limit = 1;
    }
    if (Limit > 500)
    {
        Limit = 500;
    }
    static const char* kChronological =
        "SELECT id, character_id, player_id, content, keywords, importance, embedding, embedding_dim, created_at, last_used_at, source "
        "FROM salient_memories WHERE character_id = ? AND player_id = ? ORDER BY created_at ASC, id ASC LIMIT ?;";
    return CollectMemories(ImplPtr->Db, kChronological, CharacterId, PlayerId, Limit, OutMemories);
}

Status MemoryStore::ListRelationships(std::vector<RelationshipRecord>& OutRecords)
{
    std::lock_guard<std::mutex> Lock(ImplPtr->Mutex);
    OutRecords.clear();
    if (!ImplPtr->Db)
    {
        return Status::Fail("memory store is not open");
    }
    SqlStmt Stmt;
    Status Prepared = Stmt.Prepare(ImplPtr->Db,
        "SELECT character_id, player_id, trust, affinity, fear, respect, updated_at "
        "FROM relationship_scores ORDER BY character_id, player_id LIMIT 400;");
    if (!Prepared.bOk)
    {
        return Prepared;
    }
    Status StepStatus = Status::Ok();
    while (true)
    {
        const int Rc = Stmt.Step(StepStatus);
        if (Rc == SQLITE_DONE)
        {
            return Status::Ok();
        }
        if (Rc != SQLITE_ROW)
        {
            return StepStatus;
        }
        RelationshipRecord Row;
        Row.CharacterId = Stmt.ColumnText(0);
        Row.PlayerId = Stmt.ColumnText(1);
        Row.Scores.Trust = Stmt.ColumnDouble(2);
        Row.Scores.Affinity = Stmt.ColumnDouble(3);
        Row.Scores.Fear = Stmt.ColumnDouble(4);
        Row.Scores.Respect = Stmt.ColumnDouble(5);
        Row.UpdatedAt = Stmt.ColumnText(6);
        OutRecords.push_back(std::move(Row));
    }
}

Status MemoryStore::ListMemoryCandidates(const std::string& CharacterId, const std::string& PlayerId, int Limit, std::vector<SalientMemory>& OutMemories)
{
    std::lock_guard<std::mutex> Lock(ImplPtr->Mutex);
    OutMemories.clear();
    if (!ImplPtr->Db)
    {
        return Status::Fail("memory store is not open");
    }
    if (Status Pair = ValidatePair(CharacterId, PlayerId); !Pair.bOk)
    {
        return Pair;
    }
    if (Limit < 1)
    {
        Limit = 1;
    }
    if (Limit > 500)
    {
        Limit = 500;
    }
    static const char* kByImportance =
        "SELECT id, character_id, player_id, content, keywords, importance, embedding, embedding_dim, created_at, last_used_at, source "
        "FROM salient_memories WHERE character_id = ? AND player_id = ? ORDER BY importance DESC, id DESC LIMIT ?;";
    static const char* kByRecent =
        "SELECT id, character_id, player_id, content, keywords, importance, embedding, embedding_dim, created_at, last_used_at, source "
        "FROM salient_memories WHERE character_id = ? AND player_id = ? ORDER BY id DESC LIMIT ?;";
    if (Status Listed = CollectMemories(ImplPtr->Db, kByImportance, CharacterId, PlayerId, Limit, OutMemories); !Listed.bOk)
    {
        return Listed;
    }
    return CollectMemories(ImplPtr->Db, kByRecent, CharacterId, PlayerId, Limit, OutMemories);
}

Status MemoryStore::TouchMemories(const std::vector<int64_t>& Ids, const std::string& UsedAt)
{
    std::lock_guard<std::mutex> Lock(ImplPtr->Mutex);
    if (!ImplPtr->Db)
    {
        return Status::Fail("memory store is not open");
    }
    if (Ids.empty())
    {
        return Status::Ok();
    }
    const std::string Stamp = UsedAt.empty() ? UtcNow() : UsedAt;
    for (int64_t Id : Ids)
    {
        SqlStmt Stmt;
        Status Prepared = Stmt.Prepare(ImplPtr->Db, "UPDATE salient_memories SET last_used_at = ? WHERE id = ?;");
        if (!Prepared.bOk)
        {
            return Prepared;
        }
        if (Status Bound = Stmt.BindText(1, Stamp); !Bound.bOk) return Bound;
        if (Status Bound = Stmt.BindInt64(2, Id); !Bound.bOk) return Bound;
        Status StepStatus = Status::Ok();
        if (Stmt.Step(StepStatus) != SQLITE_DONE)
        {
            return StepStatus.bOk ? Status::Fail("could not mark a memory as used") : StepStatus;
        }
    }
    return Status::Ok();
}

} // namespace WanaMemory
