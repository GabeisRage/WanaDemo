#include "WanaLegacySnapshot.h"

#include "WanaMemoryJson.h"
#include "WanaMemoryUtil.h"

namespace WanaMemory
{
namespace
{

Json::Value TraitsToJson(const LegacyTraitSnapshot& Traits)
{
    Json::Value Object = Json::MakeObject();
    Object.Set("aggression", Json::MakeNumber(Traits.Aggression));
    Object.Set("fear", Json::MakeNumber(Traits.Fear));
    Object.Set("friendliness", Json::MakeNumber(Traits.Friendliness));
    Object.Set("intelligence", Json::MakeNumber(Traits.Intelligence));
    Object.Set("curiosity", Json::MakeNumber(Traits.Curiosity));
    Object.Set("loyalty", Json::MakeNumber(Traits.Loyalty));
    Object.Set("stress", Json::MakeNumber(Traits.Stress));
    Object.Set("confidence", Json::MakeNumber(Traits.Confidence));
    Object.Set("suspicion", Json::MakeNumber(Traits.Suspicion));
    return Object;
}

LegacyTraitSnapshot TraitsFromJson(const Json::Value& Object)
{
    LegacyTraitSnapshot Traits;
    Traits.Aggression = Object.GetNumber("aggression", Traits.Aggression);
    Traits.Fear = Object.GetNumber("fear", Traits.Fear);
    Traits.Friendliness = Object.GetNumber("friendliness", Traits.Friendliness);
    Traits.Intelligence = Object.GetNumber("intelligence", Traits.Intelligence);
    Traits.Curiosity = Object.GetNumber("curiosity", Traits.Curiosity);
    Traits.Loyalty = Object.GetNumber("loyalty", Traits.Loyalty);
    Traits.Stress = Object.GetNumber("stress", Traits.Stress);
    Traits.Confidence = Object.GetNumber("confidence", Traits.Confidence);
    Traits.Suspicion = Object.GetNumber("suspicion", Traits.Suspicion);
    return Traits;
}

Json::Value SeedToJson(const LegacyRelationshipSeed& Seed)
{
    Json::Value Object = Json::MakeObject();
    Object.Set("state", Json::MakeString(Seed.State));
    Object.Set("trust", Json::MakeNumber(Seed.Trust));
    Object.Set("fear", Json::MakeNumber(Seed.Fear));
    Object.Set("respect", Json::MakeNumber(Seed.Respect));
    Object.Set("attachment", Json::MakeNumber(Seed.Attachment));
    Object.Set("hostility", Json::MakeNumber(Seed.Hostility));
    return Object;
}

LegacyRelationshipSeed SeedFromJson(const Json::Value& Object)
{
    LegacyRelationshipSeed Seed;
    const std::string State = Object.GetString("state");
    if (!State.empty())
    {
        Seed.State = State;
    }
    Seed.Trust = Object.GetNumber("trust", Seed.Trust);
    Seed.Fear = Object.GetNumber("fear", Seed.Fear);
    Seed.Respect = Object.GetNumber("respect", Seed.Respect);
    Seed.Attachment = Object.GetNumber("attachment", Seed.Attachment);
    Seed.Hostility = Object.GetNumber("hostility", Seed.Hostility);
    return Seed;
}

} // namespace

std::string BuildLegacyWAIJson(const LegacyWAISnapshot& Snapshot)
{
    Json::Value Root = Json::MakeObject();
    Root.Set("snapshot_schema", Json::MakeNumber(1));
    Root.Set("emotion", Json::MakeString(Snapshot.Emotion));
    Root.Set("emotion_intensity", Json::MakeNumber(Snapshot.EmotionIntensity));
    Root.Set("traits", TraitsToJson(Snapshot.Traits));
    Json::Value Records = Json::MakeArray();
    for (const LegacyMemoryRecord& Record : Snapshot.MemoryRecords)
    {
        Json::Value Item = Json::MakeObject();
        Item.Set("content", Json::MakeString(Record.Content));
        Item.Set("type", Json::MakeString(Record.Type));
        Item.Set("emotional_weight", Json::MakeNumber(Record.EmotionalWeight));
        Item.Set("timestamp", Json::MakeNumber(Record.Timestamp));
        Records.Array.push_back(std::move(Item));
    }
    Root.Set("memory_records", std::move(Records));
    Json::Value Events = Json::MakeArray();
    for (const LegacyPersonalityEvent& Event : Snapshot.PersonalityEvents)
    {
        Json::Value Item = Json::MakeObject();
        Item.Set("summary", Json::MakeString(Event.Summary));
        Item.Set("emotional_impact", Json::MakeNumber(Event.EmotionalImpact));
        Events.Array.push_back(std::move(Item));
    }
    Root.Set("personality_events", std::move(Events));
    return Json::Stringify(Root);
}

Status ParseLegacyWAIJson(const std::string& JsonText, LegacyWAISnapshot& OutSnapshot)
{
    Json::Value Root;
    std::string Error;
    if (!Json::Parse(JsonText, Root, Error) || Root.Type != Json::Value::Kind::Object)
    {
        return Status::Fail(Error.empty() ? "WAI snapshot is not a JSON object" : Error);
    }
    OutSnapshot = LegacyWAISnapshot();
    const std::string Emotion = Root.GetString("emotion");
    if (!Emotion.empty())
    {
        OutSnapshot.Emotion = Emotion;
    }
    OutSnapshot.EmotionIntensity = Clamp01(Root.GetNumber("emotion_intensity", 0.0));
    if (const Json::Value* Traits = Root.Find("traits"))
    {
        if (Traits->Type == Json::Value::Kind::Object)
        {
            OutSnapshot.Traits = TraitsFromJson(*Traits);
        }
    }
    if (const Json::Value* Records = Root.Find("memory_records"))
    {
        if (Records->Type == Json::Value::Kind::Array)
        {
            for (const Json::Value& Item : Records->Array)
            {
                if (Item.Type != Json::Value::Kind::Object)
                {
                    continue;
                }
                LegacyMemoryRecord Record;
                Record.Content = Item.GetString("content");
                Record.Type = Item.GetString("type", "ShortTerm");
                Record.EmotionalWeight = Item.GetNumber("emotional_weight", 0.0);
                Record.Timestamp = Item.GetNumber("timestamp", 0.0);
                OutSnapshot.MemoryRecords.push_back(std::move(Record));
            }
        }
    }
    if (const Json::Value* Events = Root.Find("personality_events"))
    {
        if (Events->Type == Json::Value::Kind::Array)
        {
            for (const Json::Value& Item : Events->Array)
            {
                if (Item.Type != Json::Value::Kind::Object)
                {
                    continue;
                }
                LegacyPersonalityEvent Event;
                Event.Summary = Item.GetString("summary");
                Event.EmotionalImpact = Item.GetNumber("emotional_impact", 0.0);
                OutSnapshot.PersonalityEvents.push_back(std::move(Event));
            }
        }
    }
    return Status::Ok();
}

std::string BuildLegacyWAYJson(const LegacyWAYSnapshot& Snapshot)
{
    Json::Value Root = Json::MakeObject();
    Root.Set("snapshot_schema", Json::MakeNumber(1));
    Json::Value Signals = Json::MakeArray();
    for (const LegacyWaySignal& Signal : Snapshot.Signals)
    {
        Json::Value Item = Json::MakeObject();
        Item.Set("name", Json::MakeString(Signal.Name));
        Item.Set("weight", Json::MakeNumber(Signal.Weight));
        Signals.Array.push_back(std::move(Item));
    }
    Root.Set("signals", std::move(Signals));
    Json::Value Profiles = Json::MakeArray();
    for (const LegacyWayProfile& Profile : Snapshot.Profiles)
    {
        Json::Value Item = Json::MakeObject();
        Item.Set("target_name", Json::MakeString(Profile.TargetName));
        Item.Set("state", Json::MakeString(Profile.State));
        Item.Set("trust", Json::MakeNumber(Profile.Trust));
        Item.Set("fear", Json::MakeNumber(Profile.Fear));
        Item.Set("respect", Json::MakeNumber(Profile.Respect));
        Item.Set("attachment", Json::MakeNumber(Profile.Attachment));
        Item.Set("hostility", Json::MakeNumber(Profile.Hostility));
        Profiles.Array.push_back(std::move(Item));
    }
    Root.Set("profiles", std::move(Profiles));
    return Json::Stringify(Root);
}

Status ParseLegacyWAYJson(const std::string& JsonText, LegacyWAYSnapshot& OutSnapshot)
{
    Json::Value Root;
    std::string Error;
    if (!Json::Parse(JsonText, Root, Error) || Root.Type != Json::Value::Kind::Object)
    {
        return Status::Fail(Error.empty() ? "WAY snapshot is not a JSON object" : Error);
    }
    OutSnapshot = LegacyWAYSnapshot();
    if (const Json::Value* Signals = Root.Find("signals"))
    {
        if (Signals->Type == Json::Value::Kind::Array)
        {
            for (const Json::Value& Item : Signals->Array)
            {
                if (Item.Type != Json::Value::Kind::Object)
                {
                    continue;
                }
                LegacyWaySignal Signal;
                Signal.Name = Item.GetString("name");
                Signal.Weight = Item.GetNumber("weight", 0.0);
                OutSnapshot.Signals.push_back(std::move(Signal));
            }
        }
    }
    if (const Json::Value* Profiles = Root.Find("profiles"))
    {
        if (Profiles->Type == Json::Value::Kind::Array)
        {
            for (const Json::Value& Item : Profiles->Array)
            {
                if (Item.Type != Json::Value::Kind::Object)
                {
                    continue;
                }
                LegacyWayProfile Profile;
                Profile.TargetName = Item.GetString("target_name");
                const std::string State = Item.GetString("state");
                if (!State.empty())
                {
                    Profile.State = State;
                }
                Profile.Trust = Item.GetNumber("trust", 0.0);
                Profile.Fear = Item.GetNumber("fear", 0.0);
                Profile.Respect = Item.GetNumber("respect", 0.0);
                Profile.Attachment = Item.GetNumber("attachment", 0.0);
                Profile.Hostility = Item.GetNumber("hostility", 0.0);
                OutSnapshot.Profiles.push_back(std::move(Profile));
            }
        }
    }
    return Status::Ok();
}

std::string BuildLegacyWAMIJson(const LegacyWAMISnapshot& Snapshot)
{
    Json::Value Root = Json::MakeObject();
    Root.Set("snapshot_schema", Json::MakeNumber(1));
    Root.Set("source", Json::MakeString("UWanaIdentityComponent"));
    Root.Set("faction", Json::MakeString(Snapshot.Faction));
    Json::Value Tags = Json::MakeArray();
    for (const std::string& Tag : Snapshot.ReputationTags)
    {
        Tags.Array.push_back(Json::MakeString(Tag));
    }
    Root.Set("reputation_tags", std::move(Tags));
    Root.Set("default_seed", SeedToJson(Snapshot.DefaultSeed));
    return Json::Stringify(Root);
}

Status ParseLegacyWAMIJson(const std::string& JsonText, LegacyWAMISnapshot& OutSnapshot)
{
    Json::Value Root;
    std::string Error;
    if (!Json::Parse(JsonText, Root, Error) || Root.Type != Json::Value::Kind::Object)
    {
        return Status::Fail(Error.empty() ? "WAMI snapshot is not a JSON object" : Error);
    }
    OutSnapshot = LegacyWAMISnapshot();
    OutSnapshot.Faction = Root.GetString("faction");
    if (const Json::Value* Tags = Root.Find("reputation_tags"))
    {
        if (Tags->Type == Json::Value::Kind::Array)
        {
            for (const Json::Value& Tag : Tags->Array)
            {
                if (Tag.Type == Json::Value::Kind::String)
                {
                    OutSnapshot.ReputationTags.push_back(Tag.String);
                }
            }
        }
    }
    if (const Json::Value* Seed = Root.Find("default_seed"))
    {
        if (Seed->Type == Json::Value::Kind::Object)
        {
            OutSnapshot.DefaultSeed = SeedFromJson(*Seed);
        }
    }
    return Status::Ok();
}

RelationshipScores RelationshipFromWayProfile(const LegacyWayProfile& Profile)
{
    RelationshipScores Scores;
    Scores.Trust = Clamp01(Profile.Trust);
    Scores.Fear = Clamp01(Profile.Fear);
    Scores.Respect = Clamp01(Profile.Respect);
    Scores.Affinity = Clamp01(Clamp01(Profile.Attachment) * (1.0 - Clamp01(Profile.Hostility)));
    return Scores;
}

} // namespace WanaMemory
