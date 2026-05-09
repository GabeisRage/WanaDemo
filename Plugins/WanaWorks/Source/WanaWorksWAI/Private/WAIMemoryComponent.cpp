#include "WAIMemoryComponent.h"

#include "WanaWorksCoreModule.h"
#include "Engine/World.h"

UWAIMemoryComponent::UWAIMemoryComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UWAIMemoryComponent::StoreMemory(const FString& Content, EWAIMemoryType Type, float EmotionalWeight)
{
    FWAIMemoryRecord Record;
    Record.Content = Content;
    Record.MemoryType = Type;
    Record.EmotionalWeight = FMath::Clamp(EmotionalWeight, 0.0f, 1.0f);

    if (UWorld* World = GetWorld())
    {
        Record.Timestamp = World->GetTimeSeconds();
    }

    MemoryBank.Add(Record);
    FWanaLogger::Log(FString::Printf(TEXT("WAI Memory stored (type=%d, weight=%.2f): %s"),
        static_cast<int32>(Type), Record.EmotionalWeight, *Content));
}

void UWAIMemoryComponent::ForgetOldMemories(int32 MaxShortTermCount)
{
    if (MaxShortTermCount < 0)
    {
        return;
    }

    int32 ShortTermCount = 0;
    for (const FWAIMemoryRecord& Record : MemoryBank)
    {
        if (Record.MemoryType == EWAIMemoryType::ShortTerm)
        {
            ++ShortTermCount;
        }
    }

    while (ShortTermCount > MaxShortTermCount)
    {
        for (int32 Index = 0; Index < MemoryBank.Num(); ++Index)
        {
            if (MemoryBank[Index].MemoryType == EWAIMemoryType::ShortTerm)
            {
                MemoryBank.RemoveAt(Index);
                --ShortTermCount;
                break;
            }
        }
    }
}

TArray<FWAIMemoryRecord> UWAIMemoryComponent::GetMemoriesByType(EWAIMemoryType Type) const
{
    TArray<FWAIMemoryRecord> Result;
    for (const FWAIMemoryRecord& Record : MemoryBank)
    {
        if (Record.MemoryType == Type)
        {
            Result.Add(Record);
        }
    }
    return Result;
}

FWAIMemoryRecord UWAIMemoryComponent::GetMostRecentMemory() const
{
    if (MemoryBank.Num() == 0)
    {
        return FWAIMemoryRecord();
    }

    int32 BestIndex = 0;
    float BestTimestamp = MemoryBank[0].Timestamp;
    for (int32 Index = 1; Index < MemoryBank.Num(); ++Index)
    {
        if (MemoryBank[Index].Timestamp > BestTimestamp)
        {
            BestTimestamp = MemoryBank[Index].Timestamp;
            BestIndex = Index;
        }
    }

    return MemoryBank[BestIndex];
}
