#include "WanaEmbedding.h"
#include "WanaLegacySnapshot.h"
#include "WanaLLMProvider.h"
#include "WanaMemoryConfig.h"
#include "WanaMemoryJson.h"
#include "WanaMemoryOrchestrator.h"
#include "WanaMemoryPrompt.h"
#include "WanaMemorySchema.h"
#include "WanaMemoryStore.h"
#include "WanaMemoryUtil.h"
#include "WanaSqlite.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <unistd.h>
#include <vector>

namespace
{

int gFailed = 0;
int gPassed = 0;

void Expect(bool Condition, const char* File, int Line, const char* Text)
{
    if (Condition)
    {
        ++gPassed;
        return;
    }
    ++gFailed;
    std::fprintf(stderr, "FAIL %s:%d %s\n", File, Line, Text);
}

#define EXPECT(Cond) Expect(static_cast<bool>(Cond), __FILE__, __LINE__, #Cond)

bool Near(double Left, double Right)
{
    return std::fabs(Left - Right) < 0.000001;
}

std::string TempPath(const char* Name)
{
    return std::string("/tmp/wana_memory_") + std::to_string(static_cast<long long>(::getpid())) + "_" + Name;
}

void RemoveDb(const std::string& Path)
{
    std::remove(Path.c_str());
    std::remove((Path + "-wal").c_str());
    std::remove((Path + "-shm").c_str());
}

class MapEnv final : public WanaMemory::IEnvLookup
{
public:
    std::map<std::string, std::string> Values;
    std::string Get(const std::string& Name) const override
    {
        const std::map<std::string, std::string>::const_iterator Found = Values.find(Name);
        return Found == Values.end() ? std::string() : Found->second;
    }
};

class ScriptedProvider final : public WanaMemory::ILLMProvider
{
public:
    std::string Id = "grok";
    std::string Script;
    bool bFail = false;
    bool bDefer = false;
    int Calls = 0;
    WanaMemory::LLMRequest LastRequest;
    WanaMemory::ResultCallback<WanaMemory::LLMResult> Pending;

    std::string ProviderId() const override
    {
        return Id;
    }

    void CompleteAsync(const WanaMemory::LLMRequest& Request, WanaMemory::ResultCallback<WanaMemory::LLMResult> Callback) override
    {
        ++Calls;
        LastRequest = Request;
        if (bDefer)
        {
            Pending = std::move(Callback);
            return;
        }
        Callback(Make());
    }

    void Flush()
    {
        Pending(Make());
    }

    WanaMemory::LLMResult Make() const
    {
        if (bFail)
        {
            return WanaMemory::LLMResult::Fail("provider failed", Id);
        }
        return WanaMemory::LLMResult::Success(Script, Id);
    }
};

std::shared_ptr<WanaMemory::MemoryStore> OpenStore(const std::string& Path)
{
    std::shared_ptr<WanaMemory::MemoryStore> Store = std::make_shared<WanaMemory::MemoryStore>();
    const WanaMemory::Status Opened = Store->Open(Path);
    EXPECT(Opened.bOk);
    if (!Opened.bOk)
    {
        std::fprintf(stderr, "open error: %s\n", Opened.Message.c_str());
    }
    return Store;
}

bool ContainsText(const std::vector<WanaMemory::LLMMessage>& Messages, const std::string& Needle)
{
    for (const WanaMemory::LLMMessage& Message : Messages)
    {
        if (Message.Content.find(Needle) != std::string::npos)
        {
            return true;
        }
    }
    return false;
}

void TestSchemaAndMigration()
{
    const std::string Fresh = TempPath("fresh.db");
    RemoveDb(Fresh);
    {
        std::shared_ptr<WanaMemory::MemoryStore> Store = OpenStore(Fresh);
        EXPECT(Store->SchemaVersion() == WanaMemory::LatestSchemaVersion());
        EXPECT(Store->SchemaVersion() == 2);
        Store->Close();
        Store->Open(Fresh);
        EXPECT(Store->SchemaVersion() == 2);
    }
    sqlite3* Raw = nullptr;
    EXPECT(sqlite3_open(Fresh.c_str(), &Raw) == SQLITE_OK);
    sqlite3_stmt* Stmt = nullptr;
    EXPECT(sqlite3_prepare_v2(Raw, "SELECT COUNT(*) FROM schema_migrations;", -1, &Stmt, nullptr) == SQLITE_OK);
    EXPECT(sqlite3_step(Stmt) == SQLITE_ROW);
    EXPECT(sqlite3_column_int(Stmt, 0) == 2);
    sqlite3_finalize(Stmt);
    sqlite3_close(Raw);

    const std::string Upgraded = TempPath("upgrade.db");
    RemoveDb(Upgraded);
    EXPECT(sqlite3_open(Upgraded.c_str(), &Raw) == SQLITE_OK);
    int MigrationCount = 0;
    const WanaMemory::SchemaMigration* Migrations = WanaMemory::SchemaMigrations(MigrationCount);
    EXPECT(MigrationCount >= 1);
    char* Error = nullptr;
    EXPECT(sqlite3_exec(Raw, Migrations[0].Sql, nullptr, nullptr, &Error) == SQLITE_OK);
    sqlite3_free(Error);
    EXPECT(sqlite3_exec(Raw, "PRAGMA user_version = 1;", nullptr, nullptr, &Error) == SQLITE_OK);
    sqlite3_free(Error);
    EXPECT(sqlite3_exec(Raw,
        "INSERT INTO conversation_turns(character_id, player_id, role, content, created_at, ordinal) "
        "VALUES('Ada','Sam','user','hello-from-v1','2020-01-01T00:00:00Z',1);",
        nullptr, nullptr, &Error) == SQLITE_OK);
    sqlite3_free(Error);
    sqlite3_close(Raw);

    {
        std::shared_ptr<WanaMemory::MemoryStore> Store = OpenStore(Upgraded);
        EXPECT(Store->SchemaVersion() == 2);
        std::vector<WanaMemory::Turn> Turns;
        EXPECT(Store->GetRecentTurns("Ada", "Sam", 10, Turns).bOk);
        EXPECT(Turns.size() == 1);
        EXPECT(Turns[0].Content == "hello-from-v1");
        EXPECT(Turns[0].ProviderId.empty());
        WanaMemory::Turn Added;
        Added.CharacterId = "Ada";
        Added.PlayerId = "Sam";
        Added.Role = "assistant";
        Added.Content = "after upgrade";
        Added.ProviderId = "grok";
        std::int64_t Id = 0;
        EXPECT(Store->AddTurn(Added, Id).bOk);
        Turns.clear();
        EXPECT(Store->GetRecentTurns("Ada", "Sam", 10, Turns).bOk);
        EXPECT(Turns.size() == 2);
        EXPECT(Turns[1].ProviderId == "grok");
    }

    const std::string Future = TempPath("future.db");
    RemoveDb(Future);
    {
        std::shared_ptr<WanaMemory::MemoryStore> Store = OpenStore(Future);
        WanaMemory::Turn Added;
        Added.CharacterId = "Ada";
        Added.PlayerId = "Sam";
        Added.Role = "user";
        Added.Content = "keep me";
        std::int64_t Id = 0;
        EXPECT(Store->AddTurn(Added, Id).bOk);
        Store->Close();
    }
    EXPECT(sqlite3_open(Future.c_str(), &Raw) == SQLITE_OK);
    EXPECT(sqlite3_exec(Raw, "PRAGMA user_version = 99;", nullptr, nullptr, &Error) == SQLITE_OK);
    sqlite3_free(Error);
    sqlite3_close(Raw);
    {
        WanaMemory::MemoryStore Store;
        const WanaMemory::Status Opened = Store.Open(Future);
        EXPECT(!Opened.bOk);
    }
    EXPECT(sqlite3_open(Future.c_str(), &Raw) == SQLITE_OK);
    EXPECT(sqlite3_prepare_v2(Raw, "SELECT content FROM conversation_turns;", -1, &Stmt, nullptr) == SQLITE_OK);
    EXPECT(sqlite3_step(Stmt) == SQLITE_ROW);
    EXPECT(std::string(reinterpret_cast<const char*>(sqlite3_column_text(Stmt, 0))) == "keep me");
    sqlite3_finalize(Stmt);
    sqlite3_close(Raw);

    RemoveDb(Fresh);
    RemoveDb(Upgraded);
    RemoveDb(Future);
}

void TestStoreBasics()
{
    const std::string Path = TempPath("nested/wana_memory.db");
    RemoveDb(Path);
    std::shared_ptr<WanaMemory::MemoryStore> Store = OpenStore(Path);

    const std::string HostileId = "'; DROP TABLE conversation_turns; --";
    WanaMemory::Turn Hostile;
    Hostile.CharacterId = HostileId;
    Hostile.PlayerId = "player";
    Hostile.Role = "user";
    Hostile.Content = "still here";
    std::int64_t Id = 0;
    EXPECT(Store->AddTurn(Hostile, Id).bOk);
    WanaMemory::Turn Normal;
    Normal.CharacterId = "Mara";
    Normal.PlayerId = "Sam";
    Normal.Role = "user";
    Normal.Content = "harbor";
    EXPECT(Store->AddTurn(Normal, Id).bOk);
    int Count = 0;
    EXPECT(Store->CountTurns(HostileId, "player", Count).bOk);
    EXPECT(Count == 1);
    EXPECT(Store->CountTurns("Mara", "Sam", Count).bOk);
    EXPECT(Count == 1);
    std::vector<WanaMemory::Turn> Turns;
    EXPECT(Store->GetRecentTurns("Mara", "Sam", 10, Turns).bOk);
    EXPECT(Turns.size() == 1);
    EXPECT(Turns[0].Content == "harbor");

    WanaMemory::Turn EmptyId;
    EmptyId.PlayerId = "Sam";
    EmptyId.Role = "user";
    EmptyId.Content = "no";
    EXPECT(!Store->AddTurn(EmptyId, Id).bOk);

    const std::string Blob = "{\"role\":\"scout\",\"custom\":{\"a\":1},\"note\":\"line\"}";
    WanaMemory::IdentityState State;
    State.CharacterId = "Mara";
    State.StateKind = "wami";
    State.SchemaVersion = 3;
    State.JsonBlob = Blob;
    EXPECT(Store->SetIdentity(State).bOk);
    WanaMemory::IdentityState Loaded;
    bool bFound = false;
    EXPECT(Store->GetIdentity("Mara", "WAMI", Loaded, bFound).bOk);
    EXPECT(bFound);
    EXPECT(Loaded.SchemaVersion == 3);
    EXPECT(Loaded.JsonBlob == Blob);
    EXPECT(Loaded.StateKind == "WAMI");

    State.StateKind = "WIT";
    State.SchemaVersion = 1;
    State.JsonBlob = "{\"cover\":2}";
    EXPECT(Store->SetIdentity(State).bOk);
    std::vector<WanaMemory::IdentityState> All;
    EXPECT(Store->ListIdentity("Mara", All).bOk);
    EXPECT(All.size() == 2);

    bool bChanged = false;
    WanaMemory::RelationshipScores Scores;
    Scores.Trust = 0.8;
    Scores.Affinity = 0.2;
    Scores.Fear = 0.1;
    Scores.Respect = 0.4;
    EXPECT(Store->SetRelationship("Mara", "Sam", Scores, "manual set", "manual", bChanged).bOk);
    EXPECT(bChanged);
    bool bRow = false;
    WanaMemory::RelationshipScores Read;
    EXPECT(Store->GetRelationship("Mara", "Sam", Read, bRow).bOk);
    EXPECT(bRow);
    EXPECT(Near(Read.Trust, 0.8));
    std::vector<WanaMemory::RelationshipEvent> History;
    EXPECT(Store->GetRelationshipHistory("Mara", "Sam", 10, History).bOk);
    EXPECT(History.size() == 1);
    EXPECT(History[0].Delta.Source == "manual");

    WanaMemory::RelationshipDelta Delta;
    Delta.CharacterId = "Mara";
    Delta.PlayerId = "Sam";
    Delta.Trust = 0.9;
    Delta.MaxAbsPerAxis = 0.25;
    Delta.Source = "model";
    Delta.Reason = "capped";
    EXPECT(Store->ApplyRelationshipDelta(Delta, Read, bChanged).bOk);
    EXPECT(bChanged);
    EXPECT(Near(Read.Trust, 1.0));

    WanaMemory::SalientMemory Memory;
    Memory.CharacterId = "Mara";
    Memory.PlayerId = "Sam";
    Memory.Content = "the player saved the village";
    Memory.Importance = 0.4;
    EXPECT(Store->AddMemory(Memory, Id).bOk);
    Memory.Content = "the weather is mild today";
    Memory.Importance = 0.99;
    EXPECT(Store->AddMemory(Memory, Id).bOk);
    std::vector<WanaMemory::SalientMemory> Candidates;
    EXPECT(Store->ListMemoryCandidates("Mara", "Sam", 20, Candidates).bOk);
    const std::vector<WanaMemory::SalientMemory> Ranked = WanaMemory::RankMemories(Candidates, "saved the village", nullptr, 2);
    EXPECT(Ranked.size() == 2);
    EXPECT(Ranked[0].Content.find("village") != std::string::npos);

    RemoveDb(Path);
}

void TestPromptBudgetAndParser()
{
    WanaMemory::PromptInput Input;
    Input.CharacterId = "Mara";
    Input.PlayerId = "Sam";
    Input.PlayerMessage = "Do you remember the harbor?";
    Input.Scores.Trust = 0.62;
    Input.MaxContextChars = 2200;
    for (int Index = 0; Index < 4; ++Index)
    {
        WanaMemory::IdentityState State;
        State.CharacterId = "Mara";
        State.SchemaVersion = 1;
        State.JsonBlob = std::string(1800, 'x');
        if (Index == 0) State.StateKind = "WAMI";
        if (Index == 1) State.StateKind = "WAI";
        if (Index == 2) State.StateKind = "WAY";
        if (Index == 3) State.StateKind = "WIT";
        Input.Identities.push_back(State);
    }
    for (int Index = 0; Index < 6; ++Index)
    {
        WanaMemory::Turn TurnRow;
        TurnRow.Role = "user";
        TurnRow.Content = std::string("turn-") + std::to_string(Index) + "-" + std::string(400, 'q');
        if (Index == 0)
        {
            TurnRow.Content = "OLDEST_TURN_TOKEN " + TurnRow.Content;
        }
        if (Index == 5)
        {
            TurnRow.Content = "NEWEST_TURN_TOKEN";
        }
        Input.RecentTurns.push_back(TurnRow);
    }
    WanaMemory::SalientMemory Memory;
    Memory.Id = 7;
    Memory.Content = std::string("MEMORY_TOKEN ") + std::string(500, 'm');
    Memory.Importance = 0.9;
    Input.RankedMemories.push_back(Memory);

    const WanaMemory::AssembledPrompt Prompt = WanaMemory::AssemblePrompt(Input);
    EXPECT(Prompt.CharsUsed <= 2200);
    EXPECT(Prompt.bTruncated);
    EXPECT(ContainsText(Prompt.Messages, "trust=0.62"));
    EXPECT(ContainsText(Prompt.Messages, "Do you remember the harbor?"));
    EXPECT(ContainsText(Prompt.Messages, "NEWEST_TURN_TOKEN"));
    EXPECT(!ContainsText(Prompt.Messages, "OLDEST_TURN_TOKEN"));
    EXPECT(Prompt.Messages.front().Role == "system");
    EXPECT(Prompt.Messages.back().Role == "user");

    const char* Raw =
        "The harbor is still there.\n"
        "<wana_memory>\n"
        "{\"memory\":\"The player asked about the harbor.\",\"importance\":0.8,\"relationship_delta\":{\"trust\":0.9,\"affinity\":0,\"fear\":0,\"respect\":0}}\n"
        "</wana_memory>\n";
    const WanaMemory::ModelMemoryUpdate Update = WanaMemory::ParseModelOutput(Raw);
    EXPECT(Update.bParsed);
    EXPECT(Update.Reply == "The harbor is still there.");
    EXPECT(Update.Memory == "The player asked about the harbor.");
    EXPECT(Near(Update.Importance, 0.8));
    EXPECT(Near(Update.Delta.Trust, 0.9));

    const WanaMemory::RelationshipDelta Heuristic = WanaMemory::InferRelationshipDelta("thank you, I trust you");
    EXPECT(Near(Heuristic.Trust, 0.08));
    EXPECT(Near(Heuristic.Affinity, 0.04));

    WanaMemory::LLMRequest Request;
    Request.Model = "grok-4";
    Request.Temperature = 0.7;
    Request.MaxOutputTokens = 400;
    WanaMemory::LLMMessage Message;
    Message.Role = "user";
    Message.Content = "hello";
    Request.Messages.push_back(Message);
    const std::string Body = WanaMemory::BuildChatCompletionRequestJson(Request);
    WanaMemory::Json::Value Root;
    std::string Error;
    EXPECT(WanaMemory::Json::Parse(Body, Root, Error));
    EXPECT(Root.Find("user") == nullptr);
    EXPECT(Root.Find("api_key") == nullptr);
    EXPECT(Root.Find("store") == nullptr);
    EXPECT(Root.GetString("model") == "grok-4");

    std::string Content;
    EXPECT(WanaMemory::ParseChatCompletionBody(
        "{\"choices\":[{\"message\":{\"role\":\"assistant\",\"content\":\"Hello\"}}]}",
        200, Content, Error));
    EXPECT(Content == "Hello");
    EXPECT(!WanaMemory::ParseChatCompletionBody(
        "{\"error\":{\"message\":\"Incorrect API key\"}}",
        401, Content, Error));
    EXPECT(Error.find("Incorrect API key") != std::string::npos);
    EXPECT(Error.find("sk-") == std::string::npos);
}

void TestConfigSecrets()
{
    WanaMemory::ProviderSettings Settings;
    const WanaMemory::Status Loaded = WanaMemory::LoadProviderSettings(
        "{\"provider\":\"chatgpt\",\"api_key\":\"test-should-not-stick\",\"openai_model\":\"gpt-test\",\"embeddings\":\"local-hash\"}",
        Settings);
    EXPECT(Loaded.bOk);
    EXPECT(Settings.Provider == "openai");
    EXPECT(Settings.OpenAIModel == "gpt-test");
    EXPECT(Settings.Embeddings == "local-hash");
    EXPECT(Settings.bIgnoredEmbeddedSecret);
    EXPECT(Settings.Warning.find("test-should-not-stick") == std::string::npos);

    const std::string Written = WanaMemory::WriteProviderSettingsJson(Settings);
    EXPECT(Written.find("api_key") == std::string::npos);
    EXPECT(Written.find("test-should-not-stick") == std::string::npos);

    MapEnv Env;
    Env.Values["WANA_OPENAI_API_KEY"] = "env-openai-key";
    std::string Warning;
    const std::string FromEnv = WanaMemory::ResolveApiKey(Settings, Env, "{\"openai_api_key\":\"file-openai-key\"}", Warning);
    EXPECT(FromEnv == "env-openai-key");

    Env.Values.clear();
    const std::string FromFile = WanaMemory::ResolveApiKey(Settings, Env, "{\"openai_api_key\":\"file-openai-key\"}", Warning);
    EXPECT(FromFile == "file-openai-key");
    EXPECT(Warning.find("file-openai-key") == std::string::npos);

    std::string Endpoint;
    EXPECT(WanaMemory::ActiveEndpoint(Settings, Endpoint).bOk);
    EXPECT(Endpoint == "https://api.openai.com/v1/chat/completions");
    Settings.Provider = "grok";
    EXPECT(WanaMemory::ActiveEndpoint(Settings, Endpoint).bOk);
    EXPECT(Endpoint == "https://api.x.ai/v1/chat/completions");
    Settings.EndpointOverride = "https://example.test/v1/chat/completions?api_key=hidden";
    EXPECT(!WanaMemory::ActiveEndpoint(Settings, Endpoint).bOk);

    const std::string FilePath = TempPath("note.txt");
    std::string IoError;
    EXPECT(WanaMemory::WriteEntireFile(FilePath, "abc", IoError));
    std::string ReadBack;
    EXPECT(WanaMemory::ReadEntireFile(FilePath, ReadBack, IoError));
    EXPECT(ReadBack == "abc");
    std::remove(FilePath.c_str());
}

void TestLegacySnapshots()
{
    WanaMemory::LegacyWAISnapshot Wai;
    Wai.Emotion = "Afraid";
    Wai.EmotionIntensity = 0.25;
    Wai.Traits.Loyalty = 0.8;
    WanaMemory::LegacyMemoryRecord Record;
    Record.Content = "met at the dock";
    Record.Type = "Event";
    Record.EmotionalWeight = 0.4;
    Record.Timestamp = 3.5;
    Wai.MemoryRecords.push_back(Record);
    WanaMemory::LegacyPersonalityEvent Event;
    Event.Summary = "kept a promise";
    Event.EmotionalImpact = 0.7;
    Wai.PersonalityEvents.push_back(Event);
    WanaMemory::LegacyWAISnapshot ParsedWai;
    EXPECT(WanaMemory::ParseLegacyWAIJson(WanaMemory::BuildLegacyWAIJson(Wai), ParsedWai).bOk);
    EXPECT(ParsedWai.Emotion == "Afraid");
    EXPECT(Near(ParsedWai.EmotionIntensity, 0.25));
    EXPECT(Near(ParsedWai.Traits.Loyalty, 0.8));
    EXPECT(ParsedWai.MemoryRecords.size() == 1);
    EXPECT(ParsedWai.MemoryRecords[0].Content == "met at the dock");
    EXPECT(ParsedWai.PersonalityEvents.size() == 1);

    WanaMemory::LegacyWAYSnapshot Way;
    WanaMemory::LegacyWayProfile Profile;
    Profile.TargetName = "Sam";
    Profile.State = "Friend";
    Profile.Trust = 0.7;
    Profile.Fear = 0.1;
    Profile.Respect = 0.6;
    Profile.Attachment = 0.5;
    Profile.Hostility = 0.2;
    Way.Profiles.push_back(Profile);
    WanaMemory::LegacyWAYSnapshot ParsedWay;
    EXPECT(WanaMemory::ParseLegacyWAYJson(WanaMemory::BuildLegacyWAYJson(Way), ParsedWay).bOk);
    EXPECT(ParsedWay.Profiles.size() == 1);
    EXPECT(ParsedWay.Profiles[0].State == "Friend");
    const WanaMemory::RelationshipScores Derived = WanaMemory::RelationshipFromWayProfile(ParsedWay.Profiles[0]);
    EXPECT(Near(Derived.Trust, 0.7));
    EXPECT(Near(Derived.Affinity, 0.4));

    WanaMemory::LegacyWAMISnapshot Wami;
    Wami.Faction = "Harbor";
    Wami.ReputationTags.push_back("scout");
    Wami.DefaultSeed.State = "Neutral";
    Wami.DefaultSeed.Trust = 0.2;
    WanaMemory::LegacyWAMISnapshot ParsedWami;
    EXPECT(WanaMemory::ParseLegacyWAMIJson(WanaMemory::BuildLegacyWAMIJson(Wami), ParsedWami).bOk);
    EXPECT(ParsedWami.Faction == "Harbor");
    EXPECT(ParsedWami.ReputationTags.size() == 1);
    EXPECT(ParsedWami.ReputationTags[0] == "scout");
    EXPECT(Near(ParsedWami.DefaultSeed.Trust, 0.2));
}

void TestEmbeddings()
{
    WanaMemory::LocalHashEmbedding Embedder;
    std::vector<float> Harbor;
    std::vector<float> HarborAgain;
    std::vector<float> Close;
    std::vector<float> Other;
    EXPECT(Embedder.Embed("harbor night watch", Harbor));
    EXPECT(Embedder.Embed("harbor night watch", HarborAgain));
    EXPECT(Embedder.Embed("harbor night", Close));
    EXPECT(Embedder.Embed("bakery invoice flour", Other));
    EXPECT(static_cast<int>(Harbor.size()) == WanaMemory::LocalHashEmbedding::kDimension);
    EXPECT(Near(WanaMemory::CosineSimilarity(Harbor, HarborAgain), 1.0));
    EXPECT(WanaMemory::CosineSimilarity(Harbor, Close) > WanaMemory::CosineSimilarity(Harbor, Other));

    WanaMemory::SalientMemory Left;
    Left.Id = 1;
    Left.Importance = 0.5;
    Left.Content = "unrelated ledger";
    Left.Embedding = Other;
    WanaMemory::SalientMemory Right;
    Right.Id = 2;
    Right.Importance = 0.5;
    Right.Content = "another ledger";
    Right.Embedding = Close;
    const std::vector<WanaMemory::SalientMemory> Ranked = WanaMemory::RankMemories(
        std::vector<WanaMemory::SalientMemory>{Left, Right},
        "harbor night watch",
        &Harbor,
        1);
    EXPECT(Ranked.size() == 1);
    EXPECT(Ranked[0].Id == 2);
}

void TestConversationFlow()
{
    const std::string Path = TempPath("talk.db");
    RemoveDb(Path);
    std::shared_ptr<WanaMemory::MemoryStore> Store = OpenStore(Path);
    WanaMemory::IdentityState Wami;
    Wami.CharacterId = "Mara";
    Wami.StateKind = "WAMI";
    Wami.SchemaVersion = 1;
    Wami.JsonBlob = "{\"role\":\"harbor scout\"}";
    EXPECT(Store->SetIdentity(Wami).bOk);

    ScriptedProvider First;
    First.Id = "grok";
    First.Script =
        "The harbor is still there.\n"
        "<wana_memory>\n"
        "{\"memory\":\"The player asked about the harbor.\",\"importance\":0.8,"
        "\"relationship_delta\":{\"trust\":0.9,\"affinity\":0,\"fear\":0,\"respect\":0}}\n"
        "</wana_memory>";

    WanaMemory::TalkRequest Request;
    Request.CharacterId = "Mara";
    Request.PlayerId = "Sam";
    Request.PlayerMessage = "Do you remember the harbor?";
    Request.Model = "grok-4";
    Request.MaxContextChars = 8000;

    WanaMemory::TalkResult FirstResult;
    bool bDone = false;
    WanaMemory::TalkAsync(Store, Request, First, std::shared_ptr<WanaMemory::IEmbeddingProvider>(),
        WanaMemory::ResultCallback<WanaMemory::TalkResult>([&FirstResult, &bDone](const WanaMemory::TalkResult& Result)
        {
            FirstResult = Result;
            bDone = true;
        }));
    EXPECT(bDone);
    EXPECT(FirstResult.Outcome.bOk);
    EXPECT(FirstResult.Reply == "The harbor is still there.");
    EXPECT(FirstResult.bMemoryWritten);
    EXPECT(FirstResult.bRelationshipChanged);
    EXPECT(Near(FirstResult.Scores.Trust, 0.75));
    EXPECT(First.Calls == 1);
    EXPECT(ContainsText(First.LastRequest.Messages, "harbor scout"));
    EXPECT(!ContainsText(First.LastRequest.Messages, Path));

    std::vector<WanaMemory::SalientMemory> Memories;
    EXPECT(Store->ListMemoryCandidates("Mara", "Sam", 10, Memories).bOk);
    EXPECT(!Memories.empty());
    EXPECT(Memories[0].Content.find("harbor") != std::string::npos);
    EXPECT(Near(Memories[0].Importance, 0.8));

    ScriptedProvider Second;
    Second.Id = "openai";
    Second.bDefer = true;
    Second.Script = "You are welcome.";
    Request.PlayerMessage = "thank you, I trust you";
    Request.Model = "gpt-test";
    bDone = false;
    WanaMemory::TalkResult SecondResult;
    WanaMemory::TalkAsync(Store, Request, Second, std::shared_ptr<WanaMemory::IEmbeddingProvider>(),
        WanaMemory::ResultCallback<WanaMemory::TalkResult>([&SecondResult, &bDone](const WanaMemory::TalkResult& Result)
        {
            SecondResult = Result;
            bDone = true;
        }));
    EXPECT(!bDone);
    EXPECT(Second.Calls == 1);
    int Count = 0;
    EXPECT(Store->CountTurns("Mara", "Sam", Count).bOk);
    EXPECT(Count == 3);
    EXPECT(ContainsText(Second.LastRequest.Messages, "The harbor is still there."));
    Second.Flush();
    EXPECT(bDone);
    EXPECT(SecondResult.Outcome.bOk);
    EXPECT(SecondResult.Reply == "You are welcome.");
    EXPECT(Near(SecondResult.Scores.Trust, 0.83));
    EXPECT(Near(SecondResult.Scores.Affinity, 0.54));
    EXPECT(Store->CountTurns("Mara", "Sam", Count).bOk);
    EXPECT(Count == 4);

    std::vector<WanaMemory::Turn> Turns;
    EXPECT(Store->GetRecentTurns("Mara", "Sam", 10, Turns).bOk);
    EXPECT(Turns.size() == 4);
    EXPECT(Turns[1].ProviderId == "grok");
    EXPECT(Turns[3].ProviderId == "openai");

    ScriptedProvider Polite;
    Polite.Id = "grok";
    Polite.Script =
        "Glad to help.\n<wana_memory>{\"memory\":\"The player was polite.\",\"importance\":0.4,"
        "\"relationship_delta\":{\"trust\":0,\"affinity\":0,\"fear\":0,\"respect\":0}}</wana_memory>";
    Request.PlayerMessage = "thank you";
    bDone = false;
    WanaMemory::TalkResult PoliteResult;
    WanaMemory::TalkAsync(Store, Request, Polite, std::shared_ptr<WanaMemory::IEmbeddingProvider>(),
        WanaMemory::ResultCallback<WanaMemory::TalkResult>([&PoliteResult, &bDone](const WanaMemory::TalkResult& Result)
        {
            PoliteResult = Result;
            bDone = true;
        }));
    EXPECT(bDone);
    EXPECT(PoliteResult.Outcome.bOk);
    EXPECT(!PoliteResult.bRelationshipChanged);
    EXPECT(Near(PoliteResult.Scores.Trust, 0.83));

    ScriptedProvider Failed;
    Failed.bFail = true;
    Request.PlayerMessage = "are you there?";
    bDone = false;
    WanaMemory::TalkResult FailedResult;
    EXPECT(Store->CountTurns("Mara", "Sam", Count).bOk);
    const int BeforeFail = Count;
    WanaMemory::TalkAsync(Store, Request, Failed, nullptr,
        WanaMemory::ResultCallback<WanaMemory::TalkResult>([&FailedResult, &bDone](const WanaMemory::TalkResult& Result)
        {
            FailedResult = Result;
            bDone = true;
        }));
    EXPECT(bDone);
    EXPECT(!FailedResult.Outcome.bOk);
    EXPECT(Store->CountTurns("Mara", "Sam", Count).bOk);
    EXPECT(Count == BeforeFail + 1);

    auto Embeddings = std::make_shared<WanaMemory::LocalHashEmbedding>();
    ScriptedProvider Embedded;
    Embedded.Script = "I kept that.";
    Request.PlayerMessage = "Remember the bakery invoice please";
    bDone = false;
    WanaMemory::TalkResult EmbeddedResult;
    WanaMemory::TalkAsync(Store, Request, Embedded, Embeddings,
        WanaMemory::ResultCallback<WanaMemory::TalkResult>([&EmbeddedResult, &bDone](const WanaMemory::TalkResult& Result)
        {
            EmbeddedResult = Result;
            bDone = true;
        }));
    EXPECT(bDone);
    EXPECT(EmbeddedResult.bMemoryWritten);
    Memories.clear();
    EXPECT(Store->ListMemoryCandidates("Mara", "Sam", 20, Memories).bOk);
    bool bSawEmbedding = false;
    for (const WanaMemory::SalientMemory& Memory : Memories)
    {
        if (Memory.Content.find("bakery") != std::string::npos || Memory.Content.find("invoice") != std::string::npos)
        {
            bSawEmbedding = Memory.EmbeddingDim == WanaMemory::LocalHashEmbedding::kDimension;
        }
    }
    EXPECT(bSawEmbedding);

    RemoveDb(Path);
}

} // namespace

int main()
{
    TestSchemaAndMigration();
    TestStoreBasics();
    TestPromptBudgetAndParser();
    TestConfigSecrets();
    TestLegacySnapshots();
    TestEmbeddings();
    TestConversationFlow();
    std::printf("wana memory tests: %d passed, %d failed\n", gPassed, gFailed);
    return gFailed == 0 ? 0 : 1;
}
