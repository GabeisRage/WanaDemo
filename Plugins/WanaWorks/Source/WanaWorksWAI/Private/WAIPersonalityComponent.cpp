#include "WAIPersonalityComponent.h"

#include "WanaWorksCoreModule.h"

UWAIPersonalityComponent::UWAIPersonalityComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UWAIPersonalityComponent::RememberEvent(const FString& Summary, float EmotionalImpact)
{
    FWAIMemoryEvent Event;
    Event.Summary = Summary;
    Event.EmotionalImpact = EmotionalImpact;
    Memories.Add(Event);
}

void UWAIPersonalityComponent::AdjustTrait(FName TraitName, float Delta)
{
    float* TraitPtr = nullptr;
    const FString TraitString = TraitName.ToString();

    if (TraitString.Equals(TEXT("Aggression"), ESearchCase::IgnoreCase))       { TraitPtr = &Traits.Aggression; }
    else if (TraitString.Equals(TEXT("Fear"), ESearchCase::IgnoreCase))        { TraitPtr = &Traits.Fear; }
    else if (TraitString.Equals(TEXT("Friendliness"), ESearchCase::IgnoreCase)){ TraitPtr = &Traits.Friendliness; }
    else if (TraitString.Equals(TEXT("Intelligence"), ESearchCase::IgnoreCase)){ TraitPtr = &Traits.Intelligence; }
    else if (TraitString.Equals(TEXT("Curiosity"), ESearchCase::IgnoreCase))   { TraitPtr = &Traits.Curiosity; }
    else if (TraitString.Equals(TEXT("Loyalty"), ESearchCase::IgnoreCase))     { TraitPtr = &Traits.Loyalty; }
    else if (TraitString.Equals(TEXT("Stress"), ESearchCase::IgnoreCase))      { TraitPtr = &Traits.Stress; }
    else if (TraitString.Equals(TEXT("Confidence"), ESearchCase::IgnoreCase))  { TraitPtr = &Traits.Confidence; }
    else if (TraitString.Equals(TEXT("Suspicion"), ESearchCase::IgnoreCase))   { TraitPtr = &Traits.Suspicion; }

    if (!TraitPtr)
    {
        FWanaLogger::Warning(FString::Printf(TEXT("AdjustTrait: unknown trait '%s'"), *TraitString));
        return;
    }

    *TraitPtr = FMath::Clamp(*TraitPtr + Delta, 0.0f, 1.0f);
}

FName UWAIPersonalityComponent::GetDominantTrait() const
{
    struct FTraitEntry
    {
        FName Name;
        float Value;
    };

    const FTraitEntry Entries[] = {
        { FName("Aggression"),    Traits.Aggression },
        { FName("Fear"),          Traits.Fear },
        { FName("Friendliness"),  Traits.Friendliness },
        { FName("Intelligence"),  Traits.Intelligence },
        { FName("Curiosity"),     Traits.Curiosity },
        { FName("Loyalty"),       Traits.Loyalty },
        { FName("Stress"),        Traits.Stress },
        { FName("Confidence"),    Traits.Confidence },
        { FName("Suspicion"),     Traits.Suspicion }
    };

    FName DominantName = Entries[0].Name;
    float DominantValue = Entries[0].Value;
    for (int32 i = 1; i < UE_ARRAY_COUNT(Entries); ++i)
    {
        if (Entries[i].Value > DominantValue)
        {
            DominantValue = Entries[i].Value;
            DominantName = Entries[i].Name;
        }
    }
    return DominantName;
}
