#pragma once

#include "WanaMemoryStatus.h"
#include "WanaMemoryTypes.h"

#include <string>
#include <vector>

namespace WanaMemory
{

/* WAMI assumption: WAMI is the character's durable self-model (role, faction,
   reputation, values). It is stored beside WAI, not inside it. The legacy
   adapter copies UWanaIdentityComponent into this blob when asked. */

struct LegacyMemoryRecord
{
    std::string Content;
    std::string Type = "ShortTerm";
    double EmotionalWeight = 0.0;
    double Timestamp = 0.0;
};

struct LegacyPersonalityEvent
{
    std::string Summary;
    double EmotionalImpact = 0.0;
};

struct LegacyTraitSnapshot
{
    double Aggression = 0.0;
    double Fear = 0.0;
    double Friendliness = 0.5;
    double Intelligence = 0.5;
    double Curiosity = 0.5;
    double Loyalty = 0.5;
    double Stress = 0.0;
    double Confidence = 0.5;
    double Suspicion = 0.0;
};

struct LegacyWAISnapshot
{
    std::string Emotion = "Calm";
    double EmotionIntensity = 0.0;
    LegacyTraitSnapshot Traits;
    std::vector<LegacyMemoryRecord> MemoryRecords;
    std::vector<LegacyPersonalityEvent> PersonalityEvents;
};

struct LegacyWaySignal
{
    std::string Name;
    double Weight = 0.0;
};

struct LegacyWayProfile
{
    std::string TargetName;
    std::string State = "Neutral";
    double Trust = 0.0;
    double Fear = 0.0;
    double Respect = 0.0;
    double Attachment = 0.0;
    double Hostility = 0.0;
};

struct LegacyWAYSnapshot
{
    std::vector<LegacyWaySignal> Signals;
    std::vector<LegacyWayProfile> Profiles;
};

struct LegacyRelationshipSeed
{
    std::string State = "Neutral";
    double Trust = 0.0;
    double Fear = 0.0;
    double Respect = 0.0;
    double Attachment = 0.0;
    double Hostility = 0.0;
};

struct LegacyWAMISnapshot
{
    std::string Faction;
    std::vector<std::string> ReputationTags;
    LegacyRelationshipSeed DefaultSeed;
};

std::string BuildLegacyWAIJson(const LegacyWAISnapshot& Snapshot);
Status ParseLegacyWAIJson(const std::string& JsonText, LegacyWAISnapshot& OutSnapshot);

std::string BuildLegacyWAYJson(const LegacyWAYSnapshot& Snapshot);
Status ParseLegacyWAYJson(const std::string& JsonText, LegacyWAYSnapshot& OutSnapshot);

std::string BuildLegacyWAMIJson(const LegacyWAMISnapshot& Snapshot);
Status ParseLegacyWAMIJson(const std::string& JsonText, LegacyWAMISnapshot& OutSnapshot);

/* WAY has attachment and hostility rather than affinity. Affinity is derived
   only when a designer explicitly imports a WAY profile. */
RelationshipScores RelationshipFromWayProfile(const LegacyWayProfile& Profile);

} // namespace WanaMemory
