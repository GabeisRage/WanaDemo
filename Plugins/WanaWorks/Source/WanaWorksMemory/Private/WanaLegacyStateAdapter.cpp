#include "WanaLegacyStateAdapter.h"

#include "GameFramework/Actor.h"
#include "UObject/UnrealType.h"

#include "WanaLegacySnapshot.h"

#include <initializer_list>

namespace
{

FString FromUtf8(const std::string& Text)
{
    return FString(UTF8_TO_TCHAR(Text.c_str()));
}

std::string ToUtf8(const FString& Text)
{
    return std::string(TCHAR_TO_UTF8(*Text));
}

const TCHAR* EmotionToText(EWAIEmotionState Emotion)
{
    switch (Emotion)
    {
    case EWAIEmotionState::Happy: return TEXT("Happy");
    case EWAIEmotionState::Afraid: return TEXT("Afraid");
    case EWAIEmotionState::Angry: return TEXT("Angry");
    case EWAIEmotionState::Stressed: return TEXT("Stressed");
    case EWAIEmotionState::Confused: return TEXT("Confused");
    case EWAIEmotionState::Hostile: return TEXT("Hostile");
    case EWAIEmotionState::Trusting: return TEXT("Trusting");
    case EWAIEmotionState::Calm:
    default: return TEXT("Calm");
    }
}

bool TextToEmotion(const FString& Text, EWAIEmotionState& OutEmotion)
{
    if (Text.Equals(TEXT("Happy"), ESearchCase::IgnoreCase)) { OutEmotion = EWAIEmotionState::Happy; return true; }
    if (Text.Equals(TEXT("Afraid"), ESearchCase::IgnoreCase)) { OutEmotion = EWAIEmotionState::Afraid; return true; }
    if (Text.Equals(TEXT("Angry"), ESearchCase::IgnoreCase)) { OutEmotion = EWAIEmotionState::Angry; return true; }
    if (Text.Equals(TEXT("Stressed"), ESearchCase::IgnoreCase)) { OutEmotion = EWAIEmotionState::Stressed; return true; }
    if (Text.Equals(TEXT("Confused"), ESearchCase::IgnoreCase)) { OutEmotion = EWAIEmotionState::Confused; return true; }
    if (Text.Equals(TEXT("Hostile"), ESearchCase::IgnoreCase)) { OutEmotion = EWAIEmotionState::Hostile; return true; }
    if (Text.Equals(TEXT("Trusting"), ESearchCase::IgnoreCase)) { OutEmotion = EWAIEmotionState::Trusting; return true; }
    if (Text.Equals(TEXT("Calm"), ESearchCase::IgnoreCase)) { OutEmotion = EWAIEmotionState::Calm; return true; }
    return false;
}

const TCHAR* MemoryTypeToText(EWAIMemoryType Type)
{
    switch (Type)
    {
    case EWAIMemoryType::LongTerm: return TEXT("LongTerm");
    case EWAIMemoryType::Combat: return TEXT("Combat");
    case EWAIMemoryType::Location: return TEXT("Location");
    case EWAIMemoryType::Event: return TEXT("Event");
    case EWAIMemoryType::PlayerRelationship: return TEXT("PlayerRelationship");
    case EWAIMemoryType::ShortTerm:
    default: return TEXT("ShortTerm");
    }
}

bool TextToMemoryType(const FString& Text, EWAIMemoryType& OutType)
{
    if (Text.Equals(TEXT("LongTerm"), ESearchCase::IgnoreCase)) { OutType = EWAIMemoryType::LongTerm; return true; }
    if (Text.Equals(TEXT("Combat"), ESearchCase::IgnoreCase)) { OutType = EWAIMemoryType::Combat; return true; }
    if (Text.Equals(TEXT("Location"), ESearchCase::IgnoreCase)) { OutType = EWAIMemoryType::Location; return true; }
    if (Text.Equals(TEXT("Event"), ESearchCase::IgnoreCase)) { OutType = EWAIMemoryType::Event; return true; }
    if (Text.Equals(TEXT("PlayerRelationship"), ESearchCase::IgnoreCase)) { OutType = EWAIMemoryType::PlayerRelationship; return true; }
    if (Text.Equals(TEXT("ShortTerm"), ESearchCase::IgnoreCase)) { OutType = EWAIMemoryType::ShortTerm; return true; }
    return false;
}

const TCHAR* RelationshipToText(EWAYRelationshipState State)
{
    switch (State)
    {
    case EWAYRelationshipState::Acquaintance: return TEXT("Acquaintance");
    case EWAYRelationshipState::Friend: return TEXT("Friend");
    case EWAYRelationshipState::Partner: return TEXT("Partner");
    case EWAYRelationshipState::Enemy: return TEXT("Enemy");
    case EWAYRelationshipState::Neutral:
    default: return TEXT("Neutral");
    }
}

bool TextToRelationship(const FString& Text, EWAYRelationshipState& OutState)
{
    if (Text.Equals(TEXT("Acquaintance"), ESearchCase::IgnoreCase)) { OutState = EWAYRelationshipState::Acquaintance; return true; }
    if (Text.Equals(TEXT("Friend"), ESearchCase::IgnoreCase)) { OutState = EWAYRelationshipState::Friend; return true; }
    if (Text.Equals(TEXT("Partner"), ESearchCase::IgnoreCase)) { OutState = EWAYRelationshipState::Partner; return true; }
    if (Text.Equals(TEXT("Enemy"), ESearchCase::IgnoreCase)) { OutState = EWAYRelationshipState::Enemy; return true; }
    if (Text.Equals(TEXT("Neutral"), ESearchCase::IgnoreCase)) { OutState = EWAYRelationshipState::Neutral; return true; }
    return false;
}

bool SetStructNumber(void* StructPtr, UScriptStruct* Struct, const TCHAR* Name, double Value)
{
    FProperty* Property = FindFProperty<FProperty>(Struct, Name);
    FNumericProperty* Numeric = Property ? CastField<FNumericProperty>(Property) : nullptr;
    if (!Numeric || !Numeric->IsFloatingPoint())
    {
        return false;
    }
    Numeric->SetFloatingPointPropertyValue(Numeric->ContainerPtrToValuePtr<void>(StructPtr), Value);
    return true;
}

bool SetStructString(void* StructPtr, UScriptStruct* Struct, const TCHAR* Name, const FString& Value)
{
    FStrProperty* Property = FindFProperty<FStrProperty>(Struct, Name);
    if (!Property)
    {
        return false;
    }
    Property->SetPropertyValue_InContainer(StructPtr, Value);
    return true;
}

bool SetStructEnum(void* StructPtr, UScriptStruct* Struct, const TCHAR* Name, int64 Value)
{
    if (FEnumProperty* Property = FindFProperty<FEnumProperty>(Struct, Name))
    {
        void* Address = Property->ContainerPtrToValuePtr<void>(StructPtr);
        Property->GetUnderlyingProperty()->SetIntPropertyValue(Address, Value);
        return true;
    }
    if (FByteProperty* Property = FindFProperty<FByteProperty>(Struct, Name))
    {
        Property->SetPropertyValue_InContainer(StructPtr, static_cast<uint8>(Value));
        return true;
    }
    return false;
}

bool StructHasFields(UScriptStruct* Struct, std::initializer_list<const TCHAR*> Names)
{
    if (!Struct)
    {
        return false;
    }
    for (const TCHAR* Name : Names)
    {
        if (!FindFProperty<FProperty>(Struct, Name))
        {
            return false;
        }
    }
    return true;
}

struct FStructArrayBinding
{
    FArrayProperty* Array = nullptr;
    FStructProperty* Struct = nullptr;
};

FStructArrayBinding FindStructArray(UObject* Object, const TCHAR* PropertyName)
{
    FStructArrayBinding Binding;
    Binding.Array = Object ? FindFProperty<FArrayProperty>(Object->GetClass(), PropertyName) : nullptr;
    Binding.Struct = Binding.Array ? CastField<FStructProperty>(Binding.Array->Inner) : nullptr;
    return Binding;
}

void CopyTraitsToSnapshot(const FWAITraitSet& Traits, WanaMemory::LegacyTraitSnapshot& OutTraits)
{
    OutTraits.Aggression = Traits.Aggression;
    OutTraits.Fear = Traits.Fear;
    OutTraits.Friendliness = Traits.Friendliness;
    OutTraits.Intelligence = Traits.Intelligence;
    OutTraits.Curiosity = Traits.Curiosity;
    OutTraits.Loyalty = Traits.Loyalty;
    OutTraits.Stress = Traits.Stress;
    OutTraits.Confidence = Traits.Confidence;
    OutTraits.Suspicion = Traits.Suspicion;
}

void CopyTraitsFromSnapshot(const WanaMemory::LegacyTraitSnapshot& Traits, FWAITraitSet& OutTraits)
{
    OutTraits.Aggression = static_cast<float>(Traits.Aggression);
    OutTraits.Fear = static_cast<float>(Traits.Fear);
    OutTraits.Friendliness = static_cast<float>(Traits.Friendliness);
    OutTraits.Intelligence = static_cast<float>(Traits.Intelligence);
    OutTraits.Curiosity = static_cast<float>(Traits.Curiosity);
    OutTraits.Loyalty = static_cast<float>(Traits.Loyalty);
    OutTraits.Stress = static_cast<float>(Traits.Stress);
    OutTraits.Confidence = static_cast<float>(Traits.Confidence);
    OutTraits.Suspicion = static_cast<float>(Traits.Suspicion);
}

bool LoadExistingWAI(UWanaMemorySubsystem* Memory, const FString& CharacterId, WanaMemory::LegacyWAISnapshot& OutSnapshot)
{
    FString Json;
    int32 Version = 0;
    FString Error;
    if (!Memory->GetIdentityState(CharacterId, TEXT("WAI"), Json, Version, Error))
    {
        return false;
    }
    return WanaMemory::ParseLegacyWAIJson(ToUtf8(Json), OutSnapshot).bOk;
}

} // namespace

bool UWanaLegacyStateAdapter::SaveLegacySnapshot(
    UWanaMemorySubsystem* Memory,
    FString CharacterId,
    UWAIMemoryComponent* MemoryComponent,
    UWAIEmotionComponent* EmotionComponent,
    UWAIPersonalityComponent* PersonalityComponent,
    UWAYPlayerProfileComponent* WayComponent,
    UWanaIdentityComponent* IdentityComponent,
    FString& OutReport)
{
    OutReport.Reset();
    if (!Memory || !Memory->IsDatabaseOpen())
    {
        OutReport = TEXT("The memory database is not open.");
        return false;
    }
    if (CharacterId.TrimStartAndEnd().IsEmpty())
    {
        OutReport = TEXT("Character id is required.");
        return false;
    }
    if (!MemoryComponent && !EmotionComponent && !PersonalityComponent && !WayComponent && !IdentityComponent)
    {
        OutReport = TEXT("Pass at least one component to save.");
        return false;
    }

    bool bOk = true;
    if (MemoryComponent || EmotionComponent || PersonalityComponent)
    {
        WanaMemory::LegacyWAISnapshot Snapshot;
        LoadExistingWAI(Memory, CharacterId, Snapshot);
        if (EmotionComponent)
        {
            Snapshot.Emotion = ToUtf8(EmotionToText(EmotionComponent->GetCurrentEmotion()));
            Snapshot.EmotionIntensity = EmotionComponent->GetEmotionIntensity();
        }
        if (PersonalityComponent)
        {
            CopyTraitsToSnapshot(PersonalityComponent->Traits, Snapshot.Traits);
            Snapshot.PersonalityEvents.clear();
            for (const FWAIMemoryEvent& Event : PersonalityComponent->GetMemories())
            {
                WanaMemory::LegacyPersonalityEvent Copy;
                Copy.Summary = ToUtf8(Event.Summary);
                Copy.EmotionalImpact = Event.EmotionalImpact;
                Snapshot.PersonalityEvents.push_back(std::move(Copy));
            }
        }
        if (MemoryComponent)
        {
            Snapshot.MemoryRecords.clear();
            const EWAIMemoryType Types[] = {
                EWAIMemoryType::ShortTerm,
                EWAIMemoryType::LongTerm,
                EWAIMemoryType::Combat,
                EWAIMemoryType::Location,
                EWAIMemoryType::Event,
                EWAIMemoryType::PlayerRelationship
            };
            for (EWAIMemoryType Type : Types)
            {
                for (const FWAIMemoryRecord& Record : MemoryComponent->GetMemoriesByType(Type))
                {
                    WanaMemory::LegacyMemoryRecord Copy;
                    Copy.Content = ToUtf8(Record.Content);
                    Copy.Type = ToUtf8(MemoryTypeToText(Record.MemoryType));
                    Copy.EmotionalWeight = Record.EmotionalWeight;
                    Copy.Timestamp = Record.Timestamp;
                    Snapshot.MemoryRecords.push_back(std::move(Copy));
                }
            }
        }
        FString Error;
        if (Memory->SetIdentityState(CharacterId, TEXT("WAI"), 1, FromUtf8(WanaMemory::BuildLegacyWAIJson(Snapshot)), Error))
        {
            OutReport += TEXT("Saved WAI snapshot.\n");
        }
        else
        {
            bOk = false;
            OutReport += Error + TEXT("\n");
        }
    }

    if (WayComponent)
    {
        WanaMemory::LegacyWAYSnapshot Snapshot;
        for (const FWAYPreferenceSignal& Signal : WayComponent->GetSignals())
        {
            WanaMemory::LegacyWaySignal Copy;
            Copy.Name = ToUtf8(Signal.SignalName.ToString());
            Copy.Weight = Signal.Weight;
            Snapshot.Signals.push_back(std::move(Copy));
        }
        for (const FWAYRelationshipProfile& Profile : WayComponent->GetRelationshipProfiles())
        {
            WanaMemory::LegacyWayProfile Copy;
            Copy.TargetName = Profile.TargetActor ? ToUtf8(Profile.TargetActor->GetName()) : std::string();
            Copy.State = ToUtf8(RelationshipToText(Profile.RelationshipState));
            Copy.Trust = Profile.Trust;
            Copy.Fear = Profile.Fear;
            Copy.Respect = Profile.Respect;
            Copy.Attachment = Profile.Attachment;
            Copy.Hostility = Profile.Hostility;
            Snapshot.Profiles.push_back(std::move(Copy));
        }
        FString Error;
        if (Memory->SetIdentityState(CharacterId, TEXT("WAY"), 1, FromUtf8(WanaMemory::BuildLegacyWAYJson(Snapshot)), Error))
        {
            OutReport += TEXT("Saved WAY snapshot. Conversation scores were left unchanged.\n");
        }
        else
        {
            bOk = false;
            OutReport += Error + TEXT("\n");
        }
    }

    if (IdentityComponent)
    {
        WanaMemory::LegacyWAMISnapshot Snapshot;
        Snapshot.Faction = IdentityComponent->FactionTag.IsNone() ? std::string() : ToUtf8(IdentityComponent->FactionTag.ToString());
        for (const FName& Tag : IdentityComponent->ReputationTags)
        {
            Snapshot.ReputationTags.push_back(ToUtf8(Tag.ToString()));
        }
        Snapshot.DefaultSeed.State = ToUtf8(RelationshipToText(IdentityComponent->DefaultRelationshipSeed.RelationshipState));
        Snapshot.DefaultSeed.Trust = IdentityComponent->DefaultRelationshipSeed.Trust;
        Snapshot.DefaultSeed.Fear = IdentityComponent->DefaultRelationshipSeed.Fear;
        Snapshot.DefaultSeed.Respect = IdentityComponent->DefaultRelationshipSeed.Respect;
        Snapshot.DefaultSeed.Attachment = IdentityComponent->DefaultRelationshipSeed.Attachment;
        Snapshot.DefaultSeed.Hostility = IdentityComponent->DefaultRelationshipSeed.Hostility;
        FString Error;
        if (Memory->SetIdentityState(CharacterId, TEXT("WAMI"), 1, FromUtf8(WanaMemory::BuildLegacyWAMIJson(Snapshot)), Error))
        {
            OutReport += TEXT("Saved WAMI snapshot from UWanaIdentityComponent.\n");
        }
        else
        {
            bOk = false;
            OutReport += Error + TEXT("\n");
        }
    }
    return bOk;
}

bool UWanaLegacyStateAdapter::LoadLegacySnapshot(
    UWanaMemorySubsystem* Memory,
    FString CharacterId,
    UWAIMemoryComponent* MemoryComponent,
    UWAIEmotionComponent* EmotionComponent,
    UWAIPersonalityComponent* PersonalityComponent,
    UWAYPlayerProfileComponent* WayComponent,
    AActor* WayTargetActor,
    UWanaIdentityComponent* IdentityComponent,
    FString& OutReport)
{
    OutReport.Reset();
    if (!Memory || !Memory->IsDatabaseOpen())
    {
        OutReport = TEXT("The memory database is not open.");
        return false;
    }
    bool bOk = true;
    bool bApplied = false;

    if (MemoryComponent || EmotionComponent || PersonalityComponent)
    {
        FString Json;
        int32 Version = 0;
        FString Error;
        if (!Memory->GetIdentityState(CharacterId, TEXT("WAI"), Json, Version, Error))
        {
            bOk = false;
            OutReport += TEXT("No WAI snapshot to load.\n");
        }
        else
        {
            WanaMemory::LegacyWAISnapshot Snapshot;
            const WanaMemory::Status Parsed = WanaMemory::ParseLegacyWAIJson(ToUtf8(Json), Snapshot);
            if (!Parsed.bOk)
            {
                bOk = false;
                OutReport += FromUtf8(Parsed.Message) + TEXT("\n");
            }
            else
            {
                if (EmotionComponent)
                {
                    EWAIEmotionState Emotion = EWAIEmotionState::Calm;
                    if (TextToEmotion(FromUtf8(Snapshot.Emotion), Emotion))
                    {
                        EmotionComponent->SetEmotion(Emotion, static_cast<float>(Snapshot.EmotionIntensity));
                        bApplied = true;
                        OutReport += TEXT("Loaded emotion onto UWAIEmotionComponent.\n");
                    }
                }
                if (PersonalityComponent)
                {
                    CopyTraitsFromSnapshot(Snapshot.Traits, PersonalityComponent->Traits);
                    bApplied = true;
                    const FStructArrayBinding Binding = FindStructArray(PersonalityComponent, TEXT("Memories"));
                    if (Binding.Array && Binding.Struct && StructHasFields(Binding.Struct->Struct, {TEXT("Summary"), TEXT("EmotionalImpact")}))
                    {
                        FScriptArrayHelper Helper(Binding.Array, Binding.Array->ContainerPtrToValuePtr<void>(PersonalityComponent));
                        Helper.EmptyValues();
                        for (const WanaMemory::LegacyPersonalityEvent& Event : Snapshot.PersonalityEvents)
                        {
                            const int32 Index = Helper.AddValue();
                            void* Element = Helper.GetRawPtr(Index);
                            SetStructString(Element, Binding.Struct->Struct, TEXT("Summary"), FromUtf8(Event.Summary));
                            SetStructNumber(Element, Binding.Struct->Struct, TEXT("EmotionalImpact"), Event.EmotionalImpact);
                        }
                        OutReport += TEXT("Loaded personality traits and events.\n");
                    }
                    else
                    {
                        bOk = false;
                        OutReport += TEXT("Personality traits were copied. Events were left unchanged because the private array was not reflected.\n");
                    }
                }
                if (MemoryComponent)
                {
                    const FStructArrayBinding Binding = FindStructArray(MemoryComponent, TEXT("MemoryBank"));
                    if (!Binding.Array || !Binding.Struct || !StructHasFields(Binding.Struct->Struct, {TEXT("Content"), TEXT("MemoryType"), TEXT("EmotionalWeight"), TEXT("Timestamp")}))
                    {
                        bOk = false;
                        OutReport += TEXT("UWAIMemoryComponent fields were not reflected, so its bank was left unchanged.\n");
                    }
                    else
                    {
                        FScriptArrayHelper Helper(Binding.Array, Binding.Array->ContainerPtrToValuePtr<void>(MemoryComponent));
                        Helper.EmptyValues();
                        for (const WanaMemory::LegacyMemoryRecord& Record : Snapshot.MemoryRecords)
                        {
                            EWAIMemoryType Type = EWAIMemoryType::ShortTerm;
                            if (!TextToMemoryType(FromUtf8(Record.Type), Type))
                            {
                                Type = EWAIMemoryType::ShortTerm;
                            }
                            const int32 Index = Helper.AddValue();
                            void* Element = Helper.GetRawPtr(Index);
                            SetStructString(Element, Binding.Struct->Struct, TEXT("Content"), FromUtf8(Record.Content));
                            SetStructEnum(Element, Binding.Struct->Struct, TEXT("MemoryType"), static_cast<int64>(Type));
                            SetStructNumber(Element, Binding.Struct->Struct, TEXT("EmotionalWeight"), Record.EmotionalWeight);
                            SetStructNumber(Element, Binding.Struct->Struct, TEXT("Timestamp"), Record.Timestamp);
                        }
                        bApplied = true;
                        OutReport += TEXT("Loaded UWAIMemoryComponent bank.\n");
                    }
                }
            }
        }
    }

    if (WayComponent && WayTargetActor)
    {
        FString Json;
        int32 Version = 0;
        FString Error;
        if (!Memory->GetIdentityState(CharacterId, TEXT("WAY"), Json, Version, Error))
        {
            bOk = false;
            OutReport += TEXT("No WAY snapshot to load.\n");
        }
        else
        {
            WanaMemory::LegacyWAYSnapshot Snapshot;
            if (!WanaMemory::ParseLegacyWAYJson(ToUtf8(Json), Snapshot).bOk)
            {
                bOk = false;
                OutReport += TEXT("WAY snapshot could not be parsed. The component was left unchanged.\n");
            }
            else
            {
                const WanaMemory::LegacyWayProfile* Match = nullptr;
                for (const WanaMemory::LegacyWayProfile& Profile : Snapshot.Profiles)
                {
                    if (FromUtf8(Profile.TargetName).Equals(WayTargetActor->GetName(), ESearchCase::IgnoreCase))
                    {
                        Match = &Profile;
                        break;
                    }
                }
                if (!Match)
                {
                    bOk = false;
                    OutReport += TEXT("No saved WAY profile matches the target actor name.\n");
                }
                else
                {
                    EWAYRelationshipState State = EWAYRelationshipState::Neutral;
                    if (TextToRelationship(FromUtf8(Match->State), State))
                    {
                        WayComponent->SetRelationshipStateForTarget(WayTargetActor, State);
                    }
                    const FStructArrayBinding Binding = FindStructArray(WayComponent, TEXT("RelationshipProfiles"));
                    FObjectProperty* TargetProperty = Binding.Struct ? FindFProperty<FObjectProperty>(Binding.Struct->Struct, TEXT("TargetActor")) : nullptr;
                    bool bWrote = false;
                    if (Binding.Array && Binding.Struct && TargetProperty)
                    {
                        FScriptArrayHelper Helper(Binding.Array, Binding.Array->ContainerPtrToValuePtr<void>(WayComponent));
                        for (int32 Index = 0; Index < Helper.Num(); ++Index)
                        {
                            void* Element = Helper.GetRawPtr(Index);
                            if (TargetProperty->GetObjectPropertyValue_InContainer(Element) == WayTargetActor)
                            {
                                SetStructNumber(Element, Binding.Struct->Struct, TEXT("Trust"), Match->Trust);
                                SetStructNumber(Element, Binding.Struct->Struct, TEXT("Fear"), Match->Fear);
                                SetStructNumber(Element, Binding.Struct->Struct, TEXT("Respect"), Match->Respect);
                                SetStructNumber(Element, Binding.Struct->Struct, TEXT("Attachment"), Match->Attachment);
                                SetStructNumber(Element, Binding.Struct->Struct, TEXT("Hostility"), Match->Hostility);
                                bWrote = true;
                                break;
                            }
                        }
                    }
                    if (bWrote)
                    {
                        bApplied = true;
                        OutReport += TEXT("Loaded WAY profile numbers onto the target actor.\n");
                    }
                    else
                    {
                        bOk = false;
                        OutReport += TEXT("WAY relationship state was applied. Numeric fields could not be written back.\n");
                    }
                }
            }
        }
    }
    else if (WayComponent)
    {
        OutReport += TEXT("WAY component load needs a target actor. The component was left unchanged.\n");
    }

    if (IdentityComponent)
    {
        FString Json;
        int32 Version = 0;
        FString Error;
        if (!Memory->GetIdentityState(CharacterId, TEXT("WAMI"), Json, Version, Error))
        {
            bOk = false;
            OutReport += TEXT("No WAMI snapshot to load.\n");
        }
        else
        {
            WanaMemory::LegacyWAMISnapshot Snapshot;
            if (!WanaMemory::ParseLegacyWAMIJson(ToUtf8(Json), Snapshot).bOk)
            {
                bOk = false;
                OutReport += TEXT("WAMI snapshot could not be parsed. UWanaIdentityComponent was left unchanged.\n");
            }
            else
            {
                IdentityComponent->FactionTag = Snapshot.Faction.empty() ? NAME_None : FName(*FromUtf8(Snapshot.Faction));
                IdentityComponent->ReputationTags.Reset();
                for (const std::string& Tag : Snapshot.ReputationTags)
                {
                    IdentityComponent->ReputationTags.Add(FName(*FromUtf8(Tag)));
                }
                EWAYRelationshipState SeedState = EWAYRelationshipState::Neutral;
                if (TextToRelationship(FromUtf8(Snapshot.DefaultSeed.State), SeedState))
                {
                    IdentityComponent->DefaultRelationshipSeed.RelationshipState = SeedState;
                }
                IdentityComponent->DefaultRelationshipSeed.Trust = static_cast<float>(Snapshot.DefaultSeed.Trust);
                IdentityComponent->DefaultRelationshipSeed.Fear = static_cast<float>(Snapshot.DefaultSeed.Fear);
                IdentityComponent->DefaultRelationshipSeed.Respect = static_cast<float>(Snapshot.DefaultSeed.Respect);
                IdentityComponent->DefaultRelationshipSeed.Attachment = static_cast<float>(Snapshot.DefaultSeed.Attachment);
                IdentityComponent->DefaultRelationshipSeed.Hostility = static_cast<float>(Snapshot.DefaultSeed.Hostility);
                bApplied = true;
                OutReport += TEXT("Loaded WAMI snapshot onto UWanaIdentityComponent.\n");
            }
        }
    }

    if (!bApplied && bOk)
    {
        OutReport += TEXT("Nothing was loaded.\n");
        return false;
    }
    return bOk;
}

bool UWanaLegacyStateAdapter::ImportRelationshipFromWAY(
    UWanaMemorySubsystem* Memory,
    FString CharacterId,
    FString PlayerId,
    UWAYPlayerProfileComponent* WayComponent,
    AActor* PlayerActor,
    FString& OutReport)
{
    OutReport.Reset();
    if (!Memory || !WayComponent || !PlayerActor)
    {
        OutReport = TEXT("Memory, a WAY component, and the player actor are required.");
        return false;
    }
    const FWAYRelationshipProfile* Match = nullptr;
    for (const FWAYRelationshipProfile& Profile : WayComponent->GetRelationshipProfiles())
    {
        if (Profile.TargetActor == PlayerActor)
        {
            Match = &Profile;
            break;
        }
    }
    if (!Match)
    {
        OutReport = TEXT("That WAY component has no profile for the player actor.");
        return false;
    }
    WanaMemory::LegacyWayProfile Portable;
    Portable.Trust = Match->Trust;
    Portable.Fear = Match->Fear;
    Portable.Respect = Match->Respect;
    Portable.Attachment = Match->Attachment;
    Portable.Hostility = Match->Hostility;
    const WanaMemory::RelationshipScores Scores = WanaMemory::RelationshipFromWayProfile(Portable);
    FWanaRelationshipScores BlueprintScores;
    BlueprintScores.Trust = static_cast<float>(Scores.Trust);
    BlueprintScores.Affinity = static_cast<float>(Scores.Affinity);
    BlueprintScores.Fear = static_cast<float>(Scores.Fear);
    BlueprintScores.Respect = static_cast<float>(Scores.Respect);
    FString Error;
    if (!Memory->SetRelationshipScores(CharacterId, PlayerId, BlueprintScores, TEXT("Imported from WAY. Affinity is attachment times one minus hostility."), Error))
    {
        OutReport = Error;
        return false;
    }
    OutReport = FString::Printf(
        TEXT("Imported WAY scores. Trust %.2f, affinity %.2f, fear %.2f, respect %.2f."),
        BlueprintScores.Trust, BlueprintScores.Affinity, BlueprintScores.Fear, BlueprintScores.Respect);
    return true;
}
