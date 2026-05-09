#include "WAYPlayerProfileComponent.h"

#include "WanaWorksCoreModule.h"

UWAYPlayerProfileComponent::UWAYPlayerProfileComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UWAYPlayerProfileComponent::RecordBehavior(EWAYBehaviorCategory Category, float Weight, const FString& Context)
{
    FWAYBehaviorRecord Record;
    Record.Category = Category;
    Record.Weight = Weight;
    Record.Context = Context;
    BehaviorHistory.Add(Record);

    FWanaLogger::Log(FString::Printf(TEXT("WAY Behavior recorded: category=%d weight=%.2f context=%s"),
        static_cast<int32>(Category), Weight, *Context));
}

void UWAYPlayerProfileComponent::RecordPreferenceSignal(FName SignalName, float Weight)
{
    FWAYPreferenceSignal Signal;
    Signal.SignalName = SignalName;
    Signal.Weight = Weight;
    Signals.Add(Signal);
}

void UWAYPlayerProfileComponent::ResetProfile()
{
    BehaviorHistory.Reset();
    Signals.Reset();
    FWanaLogger::Log(TEXT("WAY Profile reset."));
}

EWAYBehaviorCategory UWAYPlayerProfileComponent::GetDominantPlayStyle() const
{
    const int32 NumCategories = static_cast<int32>(EWAYBehaviorCategory::Dialogue) + 1;
    TArray<float> Totals;
    Totals.SetNumZeroed(NumCategories);

    for (const FWAYBehaviorRecord& Record : BehaviorHistory)
    {
        const int32 Idx = static_cast<int32>(Record.Category);
        if (Totals.IsValidIndex(Idx))
        {
            Totals[Idx] += Record.Weight;
        }
    }

    int32 BestIndex = 0;
    float BestValue = Totals.Num() > 0 ? Totals[0] : 0.0f;
    for (int32 Index = 1; Index < Totals.Num(); ++Index)
    {
        if (Totals[Index] > BestValue)
        {
            BestValue = Totals[Index];
            BestIndex = Index;
        }
    }

    return static_cast<EWAYBehaviorCategory>(BestIndex);
}

float UWAYPlayerProfileComponent::GetBehaviorScore(EWAYBehaviorCategory Category) const
{
    float Total = 0.0f;
    for (const FWAYBehaviorRecord& Record : BehaviorHistory)
    {
        if (Record.Category == Category)
        {
            Total += Record.Weight;
        }
    }
    return Total;
}
