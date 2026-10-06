#pragma once

#include "WanaEmbedding.h"
#include "WanaLLMProvider.h"
#include "WanaMemoryCallback.h"
#include "WanaMemoryStore.h"
#include "WanaMemoryTypes.h"

#include <memory>

namespace WanaMemory
{

struct MemoryWriteNotice
{
    std::string CharacterId;
    std::string PlayerId;
    std::string Kind;
};

/* Fired after a successful studio-database write. Implementations must be
   safe to call from the provider callback thread. */
class IMemoryWriteListener
{
public:
    virtual ~IMemoryWriteListener() = default;
    virtual void OnMemoryWrite(const MemoryWriteNotice& Notice) = 0;
};

struct TalkRequest
{
    std::string CharacterId;
    std::string PlayerId;
    std::string PlayerMessage;
    std::string Model;
    double Temperature = 0.7;
    int MaxOutputTokens = 400;
    int MaxContextChars = 8000;
    int RecentTurnLimit = 16;
    int TopKMemories = 6;
    std::shared_ptr<IMemoryWriteListener> WriteListener;
};

struct TalkResult
{
    Status Outcome;
    std::string Reply;
    std::string ProviderId;
    RelationshipScores Scores;
    bool bMemoryWritten = false;
    bool bRelationshipChanged = false;
    bool bContextTruncated = false;
    std::string Warning;
};

/* Retrieves studio memory, calls the provider once, then writes the reply
   back to the studio database. The provider is not given a database handle. */
void TalkAsync(
    std::shared_ptr<MemoryStore> Store,
    TalkRequest Request,
    ILLMProvider& Provider,
    std::shared_ptr<IEmbeddingProvider> Embeddings,
    ResultCallback<TalkResult> Done);

} // namespace WanaMemory
