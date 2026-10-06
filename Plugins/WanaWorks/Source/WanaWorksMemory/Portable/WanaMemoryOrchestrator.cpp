#include "WanaMemoryOrchestrator.h"

#include "WanaMemoryPrompt.h"
#include "WanaMemoryUtil.h"

namespace WanaMemory
{
namespace
{

std::string Truncate(const std::string& Text, std::size_t Limit)
{
    if (Text.size() <= Limit)
    {
        return Text;
    }
    return Text.substr(0, Limit);
}

std::string SynthesizeMemory(const std::string& PlayerText, const std::string& Reply)
{
    return Truncate("Player said: " + Trim(PlayerText) + " Character replied: " + Trim(Reply), 320);
}

void Finish(ResultCallback<TalkResult>& Done, TalkResult Result)
{
    if (Done)
    {
        Done(Result);
    }
}

} // namespace

void TalkAsync(
    std::shared_ptr<MemoryStore> Store,
    TalkRequest Request,
    ILLMProvider& Provider,
    std::shared_ptr<IEmbeddingProvider> Embeddings,
    ResultCallback<TalkResult> Done)
{
    TalkResult Early;
    if (!Store || !Store->IsOpen())
    {
        Early.Outcome = Status::Fail("memory store is not open");
        Finish(Done, std::move(Early));
        return;
    }
    if (Status Pair = ValidatePair(Request.CharacterId, Request.PlayerId); !Pair.bOk)
    {
        Early.Outcome = Pair;
        Finish(Done, std::move(Early));
        return;
    }
    Request.PlayerMessage = Trim(Request.PlayerMessage);
    if (Request.PlayerMessage.empty())
    {
        Early.Outcome = Status::Fail("player message is empty");
        Finish(Done, std::move(Early));
        return;
    }
    if (Request.PlayerMessage.size() > static_cast<std::size_t>(kMaxContentChars))
    {
        Early.Outcome = Status::Fail("player message is too long");
        Finish(Done, std::move(Early));
        return;
    }

    std::vector<Turn> RecentTurns;
    if (Status Loaded = Store->GetRecentTurns(Request.CharacterId, Request.PlayerId, Request.RecentTurnLimit, RecentTurns); !Loaded.bOk)
    {
        Early.Outcome = Loaded;
        Finish(Done, std::move(Early));
        return;
    }
    std::vector<SalientMemory> Candidates;
    const int Pool = Request.TopKMemories * 20 < 100 ? 100 : Request.TopKMemories * 20;
    if (Status Loaded = Store->ListMemoryCandidates(Request.CharacterId, Request.PlayerId, Pool, Candidates); !Loaded.bOk)
    {
        Early.Outcome = Loaded;
        Finish(Done, std::move(Early));
        return;
    }
    bool bFoundRelationship = false;
    RelationshipScores Scores;
    if (Status Loaded = Store->GetRelationship(Request.CharacterId, Request.PlayerId, Scores, bFoundRelationship); !Loaded.bOk)
    {
        Early.Outcome = Loaded;
        Finish(Done, std::move(Early));
        return;
    }
    std::vector<RelationshipEvent> History;
    if (Status Loaded = Store->GetRelationshipHistory(Request.CharacterId, Request.PlayerId, 1, History); !Loaded.bOk)
    {
        Early.Outcome = Loaded;
        Finish(Done, std::move(Early));
        return;
    }
    std::vector<IdentityState> Identities;
    if (Status Loaded = Store->ListIdentity(Request.CharacterId, Identities); !Loaded.bOk)
    {
        Early.Outcome = Loaded;
        Finish(Done, std::move(Early));
        return;
    }

    std::vector<float> QueryEmbedding;
    const std::vector<float>* QueryEmbeddingPtr = nullptr;
    if (Embeddings && Embeddings->Embed(Request.PlayerMessage, QueryEmbedding))
    {
        QueryEmbeddingPtr = &QueryEmbedding;
    }
    const std::vector<SalientMemory> Ranked = RankMemories(Candidates, Request.PlayerMessage, QueryEmbeddingPtr, Request.TopKMemories);

    PromptInput PromptInput;
    PromptInput.CharacterId = Request.CharacterId;
    PromptInput.PlayerId = Request.PlayerId;
    PromptInput.PlayerMessage = Request.PlayerMessage;
    PromptInput.Scores = Scores;
    PromptInput.Identities = Identities;
    PromptInput.RankedMemories = Ranked;
    PromptInput.RecentTurns = RecentTurns;
    PromptInput.MaxContextChars = Request.MaxContextChars;
    if (!History.empty())
    {
        PromptInput.bHasLastChange = true;
        PromptInput.LastChange = History.front().Delta;
    }
    const AssembledPrompt Prompt = AssemblePrompt(PromptInput);

    Turn UserTurn;
    UserTurn.CharacterId = Request.CharacterId;
    UserTurn.PlayerId = Request.PlayerId;
    UserTurn.Role = "user";
    UserTurn.Content = Request.PlayerMessage;
    int64_t UserTurnId = 0;
    if (Status Stored = Store->AddTurn(UserTurn, UserTurnId); !Stored.bOk)
    {
        Early.Outcome = Stored;
        Finish(Done, std::move(Early));
        return;
    }

    LLMRequest LlmRequest;
    LlmRequest.Model = Request.Model;
    LlmRequest.Temperature = Request.Temperature;
    LlmRequest.MaxOutputTokens = Request.MaxOutputTokens;
    LlmRequest.Messages = Prompt.Messages;
    const std::string ProviderId = Provider.ProviderId();
    const bool bTruncated = Prompt.bTruncated;
    const std::vector<int64_t> UsedMemoryIds = Prompt.UsedMemoryIds;

    Provider.CompleteAsync(LlmRequest, ResultCallback<LLMResult>(
        [Store, Request, Embeddings, Done = std::move(Done), ProviderId, bTruncated, UsedMemoryIds](const LLMResult& Llm) mutable
        {
            TalkResult Result;
            Result.ProviderId = Llm.ProviderId.empty() ? ProviderId : Llm.ProviderId;
            Result.bContextTruncated = bTruncated;
            Result.Scores = DefaultRelationship();
            if (!Llm.bOk)
            {
                Result.Outcome = Status::Fail(Llm.Error.empty() ? "the language model request failed" : Llm.Error);
                Finish(Done, std::move(Result));
                return;
            }

            const ModelMemoryUpdate Parsed = ParseModelOutput(Llm.Content);
            if (Trim(Parsed.Reply).empty())
            {
                Result.Outcome = Status::Fail("the provider returned no spoken reply");
                Finish(Done, std::move(Result));
                return;
            }
            Result.Reply = Parsed.Reply;

            Turn AssistantTurn;
            AssistantTurn.CharacterId = Request.CharacterId;
            AssistantTurn.PlayerId = Request.PlayerId;
            AssistantTurn.Role = "assistant";
            AssistantTurn.Content = Truncate(Parsed.Reply, static_cast<std::size_t>(kMaxContentChars));
            AssistantTurn.ProviderId = Result.ProviderId;
            int64_t AssistantId = 0;
            if (Status Stored = Store->AddTurn(AssistantTurn, AssistantId); !Stored.bOk)
            {
                Result.Outcome = Status::Ok();
                Result.Warning = Stored.Message;
                Finish(Done, std::move(Result));
                return;
            }

            SalientMemory Memory;
            Memory.CharacterId = Request.CharacterId;
            Memory.PlayerId = Request.PlayerId;
            if (!Parsed.Memory.empty())
            {
                Memory.Content = Parsed.Memory;
                Memory.Importance = Parsed.bHasImportance ? Parsed.Importance : 0.55;
                Memory.Source = "model";
            }
            else
            {
                Memory.Content = SynthesizeMemory(Request.PlayerMessage, Parsed.Reply);
                Memory.Importance = 0.35;
                Memory.Source = "synthesized";
            }
            if (Embeddings)
            {
                std::vector<float> Vector;
                if (Embeddings->Embed(Memory.Content, Vector))
                {
                    Memory.Embedding = std::move(Vector);
                }
            }
            int64_t MemoryId = 0;
            if (Status Stored = Store->AddMemory(Memory, MemoryId); Stored.bOk)
            {
                Result.bMemoryWritten = true;
            }
            else
            {
                Result.Warning = Stored.Message;
            }

            RelationshipDelta Delta = Parsed.bParsed ? Parsed.Delta : InferRelationshipDelta(Request.PlayerMessage);
            Delta.CharacterId = Request.CharacterId;
            Delta.PlayerId = Request.PlayerId;
            if (!Parsed.bParsed)
            {
                Delta.Source = "heuristic";
                Delta.MaxAbsPerAxis = kHeuristicDeltaCap;
                if (Delta.Reason.empty())
                {
                    Delta.Reason = "local phrase heuristic";
                }
            }
            else if (Delta.Source.empty())
            {
                Delta.Source = "model";
                Delta.MaxAbsPerAxis = kModelDeltaCap;
            }
            bool bChanged = false;
            RelationshipScores Updated;
            if (Status Applied = Store->ApplyRelationshipDelta(Delta, Updated, bChanged); Applied.bOk)
            {
                Result.Scores = Updated;
                Result.bRelationshipChanged = bChanged;
            }
            else if (Result.Warning.empty())
            {
                Result.Warning = Applied.Message;
                Result.Scores = DefaultRelationship();
            }
            if (Status Touched = Store->TouchMemories(UsedMemoryIds, std::string()); !Touched.bOk && Result.Warning.empty())
            {
                Result.Warning = Touched.Message;
            }
            Result.Outcome = Status::Ok();
            if (bTruncated && Result.Warning.empty())
            {
                Result.Warning = "Context was trimmed to the character budget.";
            }
            Finish(Done, std::move(Result));
        }));
}

} // namespace WanaMemory
