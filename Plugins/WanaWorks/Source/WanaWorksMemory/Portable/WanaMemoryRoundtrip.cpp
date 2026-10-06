#include "WanaMemoryRoundtrip.h"

#include "WanaLLMProvider.h"
#include "WanaMemoryOrchestrator.h"
#include "WanaMemoryPrompt.h"
#include "WanaMemoryStore.h"

#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

namespace WanaMemory
{
namespace
{

struct Check
{
    int Failed = 0;
    std::string Text;

    void Ok(const std::string& Line)
    {
        Text += "ok " + Line + "\n";
    }

    void Fail(const std::string& Line)
    {
        Text += "FAIL " + Line + "\n";
        ++Failed;
    }

    void Expect(bool Condition, const std::string& Line)
    {
        if (Condition)
        {
            Ok(Line);
        }
        else
        {
            Fail(Line);
        }
    }
};

bool Near(double Left, double Right)
{
    return std::fabs(Left - Right) < 0.000001;
}

bool Contains(const std::string& Haystack, const char* Needle)
{
    return Needle && Haystack.find(Needle) != std::string::npos;
}

bool MessagesContain(const std::vector<LLMMessage>& Messages, const char* Needle)
{
    for (const LLMMessage& Message : Messages)
    {
        if (Contains(Message.Content, Needle))
        {
            return true;
        }
    }
    return false;
}

std::string TempDatabasePath()
{
    const char* Dir = std::getenv("TMPDIR");
    if (!Dir || Dir[0] == '\0')
    {
        Dir = std::getenv("TEMP");
    }
    if (!Dir || Dir[0] == '\0')
    {
        Dir = "/tmp";
    }
    const auto Stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    return std::string(Dir) + "/wana_memory_roundtrip_" + std::to_string(static_cast<long long>(Stamp)) + ".db";
}

void RemoveDatabase(const std::string& Path)
{
    std::remove(Path.c_str());
    std::remove((Path + "-wal").c_str());
    std::remove((Path + "-shm").c_str());
}

class MockProvider final : public ILLMProvider
{
public:
    std::string Id;
    std::string Script;
    LLMRequest LastRequest;
    int Calls = 0;

    std::string ProviderId() const override
    {
        return Id;
    }

    void CompleteAsync(const LLMRequest& Request, ResultCallback<LLMResult> Callback) override
    {
        ++Calls;
        LastRequest = Request;
        Callback(LLMResult::Success(Script, Id));
    }
};

Status WriteIdentity(MemoryStore& Store, const std::string& CharacterId, const char* Kind, const char* Json)
{
    IdentityState State;
    State.CharacterId = CharacterId;
    State.StateKind = Kind;
    State.SchemaVersion = 1;
    State.JsonBlob = Json;
    return Store.SetIdentity(State);
}

} // namespace

int RunMemoryRoundtripScenario(std::string& OutReport)
{
    Check Log;
    Log.Text = "WanaWorks memory roundtrip\n";
    const std::string Path = TempDatabasePath();
    RemoveDatabase(Path);

    const std::string CharacterId = "Lyra";
    const std::string PlayerId = "Noah";
    const char* MemoryText = "Lyra hid the lantern-under-the-dock.";
    const char* WitJson = "{\"place\":\"north-dock\"}";
    const char* WayJson = "{\"stance\":\"wary-ally\"}";
    const char* WaiJson = "{\"mood\":\"guarded-watch\"}";
    const char* WamiJson = "{\"role\":\"night-watch-lyra\"}";

    {
        MemoryStore Store;
        const Status Opened = Store.Open(Path);
        Log.Expect(Opened.bOk, "opened a temporary database");
        if (!Opened.bOk)
        {
            Log.Fail(Opened.Message);
            RemoveDatabase(Path);
            OutReport = Log.Text;
            return Log.Failed;
        }

        SalientMemory Memory;
        Memory.CharacterId = CharacterId;
        Memory.PlayerId = PlayerId;
        Memory.Content = MemoryText;
        Memory.Importance = 0.82;
        Memory.Source = "manual";
        int64_t MemoryId = 0;
        Log.Expect(Store.AddMemory(Memory, MemoryId).bOk, "wrote a salient memory");

        RelationshipScores Scores;
        Scores.Trust = 0.71;
        Scores.Affinity = 0.64;
        Scores.Fear = 0.12;
        Scores.Respect = 0.58;
        bool bChanged = false;
        Log.Expect(Store.SetRelationship(CharacterId, PlayerId, Scores, "roundtrip seed", "manual", bChanged).bOk, "wrote relationship scores");
        Log.Expect(bChanged, "relationship row was created");

        Log.Expect(WriteIdentity(Store, CharacterId, "WIT", WitJson).bOk, "wrote WIT");
        Log.Expect(WriteIdentity(Store, CharacterId, "WAY", WayJson).bOk, "wrote WAY");
        Log.Expect(WriteIdentity(Store, CharacterId, "WAI", WaiJson).bOk, "wrote WAI");
        Log.Expect(WriteIdentity(Store, CharacterId, "WAMI", WamiJson).bOk, "wrote WAMI");
        Store.Close();
    }

    std::vector<SalientMemory> Memories;
    RelationshipScores LoadedScores;
    std::vector<IdentityState> Identities;
    {
        MemoryStore View;
        const Status Opened = View.OpenView(Path);
        Log.Expect(Opened.bOk, "reopened the database read-only");
        if (!Opened.bOk)
        {
            Log.Fail(Opened.Message);
            RemoveDatabase(Path);
            OutReport = Log.Text;
            return Log.Failed;
        }

        Log.Expect(View.ListMemories(CharacterId, PlayerId, 20, Memories).bOk, "retrieved memories");
        Log.Expect(Memories.size() == 1 && Memories[0].Content == MemoryText, "memory text survived reopen");
        Log.Expect(!Memories.empty() && Near(Memories[0].Importance, 0.82), "memory importance survived reopen");

        bool bFound = false;
        Log.Expect(View.GetRelationship(CharacterId, PlayerId, LoadedScores, bFound).bOk && bFound, "retrieved relationship scores");
        Log.Expect(Near(LoadedScores.Trust, 0.71) && Near(LoadedScores.Affinity, 0.64) && Near(LoadedScores.Fear, 0.12) && Near(LoadedScores.Respect, 0.58),
            "trust, affinity, fear, and respect survived reopen");

        Log.Expect(View.ListIdentity(CharacterId, Identities).bOk, "retrieved identity state");
        bool bWit = false;
        bool bWay = false;
        bool bWai = false;
        bool bWami = false;
        for (const IdentityState& State : Identities)
        {
            bWit = bWit || (State.StateKind == "WIT" && State.JsonBlob == WitJson);
            bWay = bWay || (State.StateKind == "WAY" && State.JsonBlob == WayJson);
            bWai = bWai || (State.StateKind == "WAI" && State.JsonBlob == WaiJson);
            bWami = bWami || (State.StateKind == "WAMI" && State.JsonBlob == WamiJson);
        }
        Log.Expect(bWit && bWay && bWai && bWami, "WIT, WAY, WAI, and WAMI survived reopen");

        std::vector<RelationshipRecord> Edges;
        Log.Expect(View.ListRelationships(Edges).bOk, "listed relationship edges");
        bool bEdge = false;
        for (const RelationshipRecord& Edge : Edges)
        {
            if (Edge.CharacterId == CharacterId && Edge.PlayerId == PlayerId && Near(Edge.Scores.Trust, 0.71))
            {
                bEdge = true;
            }
        }
        Log.Expect(bEdge, "the Lyra/Noah edge is in the graph data");

        SalientMemory Rejected;
        Rejected.CharacterId = CharacterId;
        Rejected.PlayerId = PlayerId;
        Rejected.Content = "this view must not write";
        int64_t RejectedId = 0;
        Log.Expect(!View.AddMemory(Rejected, RejectedId).bOk, "read-only view rejected a write");

        const std::vector<SalientMemory> Ranked = RankMemories(Memories, "Where is the lantern?", nullptr, 4);
        PromptInput Input;
        Input.CharacterId = CharacterId;
        Input.PlayerId = PlayerId;
        Input.PlayerMessage = "Where is the lantern?";
        Input.Scores = LoadedScores;
        Input.Identities = Identities;
        Input.RankedMemories = Ranked;
        Input.MaxContextChars = 8000;
        const AssembledPrompt Prompt = AssemblePrompt(Input);
        Log.Expect(!Prompt.Messages.empty(), "assembled a prompt");
        Log.Expect(MessagesContain(Prompt.Messages, "lantern-under-the-dock"), "prompt injects the stored memory");
        Log.Expect(MessagesContain(Prompt.Messages, "north-dock"), "prompt injects WIT");
        Log.Expect(MessagesContain(Prompt.Messages, "wary-ally"), "prompt injects WAY");
        Log.Expect(MessagesContain(Prompt.Messages, "guarded-watch"), "prompt injects WAI");
        Log.Expect(MessagesContain(Prompt.Messages, "night-watch-lyra"), "prompt injects WAMI");
        Log.Expect(MessagesContain(Prompt.Messages, "trust=0.71"), "prompt injects the relationship scores");
        const std::string RequestJson = BuildChatCompletionRequestJson(LLMRequest{"grok-4", 0.7, 400, Prompt.Messages});
        Log.Expect(!Contains(RequestJson, "api_key") && !Contains(RequestJson, "\"store\""), "provider request has no key and no vendor store flag");
        View.Close();
    }

    MockProvider Grok;
    Grok.Id = "grok";
    Grok.Script =
        "The dock is quiet.\n"
        "<wana_memory>\n"
        "{\"memory\":\"Noah asked about the dock.\",\"importance\":0.4,"
        "\"relationship_delta\":{\"trust\":0,\"affinity\":0,\"fear\":0,\"respect\":0}}\n"
        "</wana_memory>";
    MockProvider OpenAI;
    OpenAI.Id = "openai";
    OpenAI.Script = "I still have the lantern.";

    {
        std::shared_ptr<MemoryStore> Store = std::make_shared<MemoryStore>();
        const Status Opened = Store->Open(Path);
        Log.Expect(Opened.bOk, "reopened the database for provider calls");
        if (!Opened.bOk)
        {
            RemoveDatabase(Path);
            OutReport = Log.Text;
            return Log.Failed;
        }

        TalkRequest First;
        First.CharacterId = CharacterId;
        First.PlayerId = PlayerId;
        First.PlayerMessage = "Where is the lantern?";
        First.Model = "grok-4";
        TalkResult FirstResult;
        bool bFirstDone = false;
        TalkAsync(Store, First, Grok, std::shared_ptr<IEmbeddingProvider>(),
            ResultCallback<TalkResult>([&FirstResult, &bFirstDone](const TalkResult& Result)
            {
                FirstResult = Result;
                bFirstDone = true;
            }));
        Log.Expect(bFirstDone && FirstResult.Outcome.bOk && FirstResult.ProviderId == "grok", "mock grok provider answered");
        Log.Expect(Grok.Calls == 1 && MessagesContain(Grok.LastRequest.Messages, "night-watch-lyra"), "grok call received the stored WAMI state");
        Log.Expect(Near(FirstResult.Scores.Trust, 0.71), "explicit zero delta left trust unchanged");

        TalkRequest Second = First;
        Second.PlayerMessage = "Where did you put it?";
        Second.Model = "gpt-4o-mini";
        TalkResult SecondResult;
        bool bSecondDone = false;
        TalkAsync(Store, Second, OpenAI, std::shared_ptr<IEmbeddingProvider>(),
            ResultCallback<TalkResult>([&SecondResult, &bSecondDone](const TalkResult& Result)
            {
                SecondResult = Result;
                bSecondDone = true;
            }));
        Log.Expect(bSecondDone && SecondResult.Outcome.bOk && SecondResult.ProviderId == "openai", "mock openai provider answered");
        Log.Expect(OpenAI.Calls == 1, "openai provider was called once");
        Log.Expect(MessagesContain(OpenAI.LastRequest.Messages, "The dock is quiet."), "swapped provider still received the stored transcript");
        Log.Expect(MessagesContain(OpenAI.LastRequest.Messages, "lantern-under-the-dock"), "swapped provider still received the stored memory");
        Log.Expect(MessagesContain(OpenAI.LastRequest.Messages, "wary-ally"), "swapped provider still received WAY");
        const std::string SecondJson = BuildChatCompletionRequestJson(OpenAI.LastRequest);
        Log.Expect(!Contains(SecondJson, "api_key") && !Contains(SecondJson, "\"store\""), "swapped provider request stays stateless");
        Store->Close();
    }

    {
        MemoryStore View;
        Log.Expect(View.OpenView(Path).bOk, "reopened after the provider swap");
        std::vector<Turn> Turns;
        Log.Expect(View.GetRecentTurns(CharacterId, PlayerId, 20, Turns).bOk, "retrieved turns after the swap");
        bool bGrok = false;
        bool bOpenAI = false;
        for (const Turn& Turn : Turns)
        {
            if (Turn.Role == "assistant" && Turn.ProviderId == "grok" && Contains(Turn.Content, "The dock is quiet."))
            {
                bGrok = true;
            }
            if (Turn.Role == "assistant" && Turn.ProviderId == "openai" && Contains(Turn.Content, "I still have the lantern."))
            {
                bOpenAI = true;
            }
        }
        Log.Expect(bGrok && bOpenAI, "assistant turns keep the provider that produced them");
        RelationshipScores After;
        bool bFound = false;
        Log.Expect(View.GetRelationship(CharacterId, PlayerId, After, bFound).bOk && bFound && Near(After.Trust, 0.71),
            "relationship scores stayed in the studio database across the provider swap");
        View.Close();
    }

    RemoveDatabase(Path);
    Log.Text += "failed " + std::to_string(Log.Failed) + "\n";
    OutReport = Log.Text;
    return Log.Failed;
}

} // namespace WanaMemory
