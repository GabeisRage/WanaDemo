#pragma once

#include "WanaMemoryTypes.h"

#include <cstdint>
#include <vector>

namespace WanaMemory
{

struct ScoredMemory
{
    SalientMemory Memory;
    double Score = 0.0;
};

struct PromptInput
{
    std::string CharacterId;
    std::string PlayerId;
    std::string PlayerMessage;
    RelationshipScores Scores;
    bool bHasLastChange = false;
    RelationshipDelta LastChange;
    std::vector<IdentityState> Identities;
    std::vector<SalientMemory> RankedMemories;
    std::vector<Turn> RecentTurns;
    int MaxContextChars = 8000;
};

struct AssembledPrompt
{
    std::string SystemPrompt;
    std::vector<LLMMessage> Messages;
    int CharsUsed = 0;
    bool bTruncated = false;
    std::vector<std::int64_t> UsedMemoryIds;
};

struct ModelMemoryUpdate
{
    bool bParsed = false;
    std::string Reply;
    std::string Memory;
    double Importance = 0.55;
    bool bHasImportance = false;
    RelationshipDelta Delta;
};

std::vector<SalientMemory> RankMemories(
    const std::vector<SalientMemory>& Candidates,
    const std::string& Query,
    const std::vector<float>* QueryEmbedding,
    int TopK);

AssembledPrompt AssemblePrompt(const PromptInput& Input);
ModelMemoryUpdate ParseModelOutput(const std::string& Raw);
RelationshipDelta InferRelationshipDelta(const std::string& PlayerText);

std::string BuildChatCompletionRequestJson(const LLMRequest& Request);
bool ParseChatCompletionBody(const std::string& Body, int HttpStatus, std::string& OutContent, std::string& OutError);

std::string FormatTranscript(const std::vector<Turn>& Turns);
std::string FormatRelationshipHistory(const std::vector<RelationshipEvent>& Events);

} // namespace WanaMemory
