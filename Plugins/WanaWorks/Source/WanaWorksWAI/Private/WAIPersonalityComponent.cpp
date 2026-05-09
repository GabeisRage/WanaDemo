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
    float* TargetTrait = nullptr;

    if (TraitName == TEXT("Aggression"))        { TargetTrait = &Traits.Aggression; }
    else if (TraitName == TEXT("Fear"))         { TargetTrait = &Traits.Fear; }
    else if (TraitName == TEXT("Friendliness")) { TargetTrait = &Traits.Friendliness; }
    else if (TraitName == TEXT("Intelligence")) { TargetTrait = &Traits.Intelligence; }
    else if (TraitName == TEXT("Curiosity"))    { TargetTrait = &Traits.Curiosity; }
    else if (TraitName == TEXT("Loyalty"))      { TargetTrait = &Traits.Loyalty; }
    else if (TraitName == TEXT("Stress"))       { TargetTrait = &Traits.Stress; }
    else if (TraitName == TEXT("Confidence"))   { TargetTrait = &Traits.Confidence; }
    else if (TraitName == TEXT("Suspicion"))    { TargetTrait = &Traits.Suspicion; }

    if (!TargetTrait)
    {
        FWanaLogger::Warning(FString::Printf(TEXT("AdjustTrait: unknown trait '%s'"), *TraitName.ToString()));
        return;
    }

    *TargetTrait = FMath::Clamp(*TargetTrait + Delta, 0.0f, 1.0f);
}

FName UWAIPersonalityComponent::GetDominantTrait() const
{
    struct FTraitEntry { FName Name; float Value; };
    const FTraitEntry Entries[] = {
        { TEXT("Aggression"),    Traits.Aggression },
        { TEXT("Fear"),          Traits.Fear },
        { TEXT("Friendliness"),  Traits.Friendliness },
        { TEXT("Intelligence"),  Traits.Intelligence },
        { TEXT("Curiosity"),     Traits.Curiosity },
        { TEXT("Loyalty"),       Traits.Loyalty },
        { TEXT("Stress"),        Traits.Stress },
        { TEXT("Confidence"),    Traits.Confidence },
        { TEXT("Suspicion"),     Traits.Suspicion }
    };

    FName Best = Entries[0].Name;
    float BestValue = Entries[0].Value;
    for (const FTraitEntry& Entry : Entries)
    {
        if (Entry.Value > BestValue)
        {
            BestValue = Entry.Value;
            Best = Entry.Name;
        }
    }
    return Best;
}
