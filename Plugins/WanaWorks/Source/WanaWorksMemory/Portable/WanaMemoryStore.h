#pragma once

#include "WanaMemoryTypes.h"

#include <memory>
#include <string>
#include <vector>

namespace WanaMemory
{

class MemoryStore
{
public:
    MemoryStore();
    ~MemoryStore();

    MemoryStore(const MemoryStore&) = delete;
    MemoryStore& operator=(const MemoryStore&) = delete;

    Status Open(const std::string& Path);
    void Close();
    bool IsOpen() const;
    int SchemaVersion() const;
    std::string Path() const;

    Status AddTurn(const Turn& InTurn, int64_t& OutId);
    Status GetRecentTurns(const std::string& CharacterId, const std::string& PlayerId, int Limit, std::vector<Turn>& OutTurns);
    Status CountTurns(const std::string& CharacterId, const std::string& PlayerId, int& OutCount);

    Status GetRelationship(const std::string& CharacterId, const std::string& PlayerId, RelationshipScores& OutScores, bool& bFound);
    Status ApplyRelationshipDelta(const RelationshipDelta& Delta, RelationshipScores& OutScores, bool& bChanged);
    Status SetRelationship(const std::string& CharacterId, const std::string& PlayerId, const RelationshipScores& Scores, const std::string& Reason, const std::string& Source, bool& bChanged);
    Status GetRelationshipHistory(const std::string& CharacterId, const std::string& PlayerId, int Limit, std::vector<RelationshipEvent>& OutEvents);

    Status SetIdentity(const IdentityState& State);
    Status GetIdentity(const std::string& CharacterId, const std::string& StateKind, IdentityState& OutState, bool& bFound);
    Status ListIdentity(const std::string& CharacterId, std::vector<IdentityState>& OutStates);

    Status AddMemory(const SalientMemory& InMemory, int64_t& OutId);
    Status ListMemoryCandidates(const std::string& CharacterId, const std::string& PlayerId, int Limit, std::vector<SalientMemory>& OutMemories);
    Status TouchMemories(const std::vector<int64_t>& Ids, const std::string& UsedAt);

private:
    struct Impl;
    std::unique_ptr<Impl> ImplPtr;
};

} // namespace WanaMemory
