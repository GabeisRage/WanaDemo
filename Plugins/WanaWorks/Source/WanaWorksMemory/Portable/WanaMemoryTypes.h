#pragma once

#include "WanaMemoryStatus.h"

#include <cstdint>
#include <string>
#include <vector>

namespace WanaMemory
{

constexpr int kMaxIdChars = 256;
constexpr int kMaxContentChars = 100000;
constexpr int kMaxIdentityChars = 256000;
constexpr double kModelDeltaCap = 0.25;
constexpr double kHeuristicDeltaCap = 0.08;

struct RelationshipScores
{
    double Trust = 0.5;
    double Affinity = 0.5;
    double Fear = 0.0;
    double Respect = 0.5;
};

inline RelationshipScores DefaultRelationship()
{
    return RelationshipScores();
}

struct RelationshipDelta
{
    std::string CharacterId;
    std::string PlayerId;
    double Trust = 0.0;
    double Affinity = 0.0;
    double Fear = 0.0;
    double Respect = 0.0;
    std::string Reason;
    std::string Source;
    double MaxAbsPerAxis = kModelDeltaCap;
};

struct RelationshipEvent
{
    int64_t Id = 0;
    std::string CharacterId;
    std::string PlayerId;
    RelationshipScores Scores;
    RelationshipDelta Delta;
    std::string CreatedAt;
};

struct RelationshipRecord
{
    std::string CharacterId;
    std::string PlayerId;
    RelationshipScores Scores;
    std::string UpdatedAt;
};

struct Turn
{
    int64_t Id = 0;
    std::string CharacterId;
    std::string PlayerId;
    std::string Role;
    std::string Content;
    std::string CreatedAt;
    int64_t Ordinal = 0;
    std::string ProviderId;
};

struct IdentityState
{
    std::string CharacterId;
    std::string StateKind;
    int SchemaVersion = 1;
    std::string JsonBlob;
    std::string UpdatedAt;
};

struct SalientMemory
{
    int64_t Id = 0;
    std::string CharacterId;
    std::string PlayerId;
    std::string Content;
    std::string Keywords;
    double Importance = 0.5;
    std::vector<float> Embedding;
    int EmbeddingDim = 0;
    std::string CreatedAt;
    std::string LastUsedAt;
    std::string Source;
};

struct LLMMessage
{
    std::string Role;
    std::string Content;
};

struct LLMRequest
{
    std::string Model;
    double Temperature = 0.7;
    int MaxOutputTokens = 400;
    std::vector<LLMMessage> Messages;
};

struct LLMResult
{
    bool bOk = false;
    std::string Content;
    std::string Error;
    std::string ProviderId;

    static LLMResult Success(std::string InContent, std::string InProviderId)
    {
        LLMResult Result;
        Result.bOk = true;
        Result.Content = std::move(InContent);
        Result.ProviderId = std::move(InProviderId);
        return Result;
    }

    static LLMResult Fail(std::string InError, std::string InProviderId = std::string())
    {
        LLMResult Result;
        Result.bOk = false;
        Result.Error = std::move(InError);
        Result.ProviderId = std::move(InProviderId);
        return Result;
    }
};

Status ValidatePair(const std::string& CharacterId, const std::string& PlayerId);
std::string NormalizeStateKind(const std::string& StateKind, Status& OutStatus);
bool IsAllowedTurnRole(const std::string& Role);

} // namespace WanaMemory
