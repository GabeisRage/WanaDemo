#include "WAIMemoryComponent.h"

#include "Engine/World.h"

void UWAIMemoryComponent::StoreMemory(FString Content, EWAIMemoryType Type, float EmotionalWeight)
{
    FWAIMemoryRecord Record;
    Record.Content = MoveTemp(Content);
    Record.MemoryType = Type;
    Record.EmotionalWeight = EmotionalWeight;
    Record.Timestamp = (GetWorld() != nullptr) ? GetWorld()->GetTimeSeconds() : 0.0f;
    MemoryBank.Add(Record);
}

void UWAIMemoryComponent::ForgetOldMemories(int32 MaxShortTermCount)
{
    if (MaxShortTermCount < 0)
    {
        return;
    }

    TArray<int32> ShortTermIndices;
    for (int32 i = 0; i < MemoryBank.Num(); ++i)
    {
        if (MemoryBank[i].MemoryType == EWAIMemoryType::ShortTerm)
        {
            ShortTermIndices.Add(i);
        }
    }

    if (ShortTermIndices.Num() <= MaxShortTermCount)
    {
        return;
    }

    ShortTermIndices.Sort([this](const int32& A, const int32& B)
    {
        return MemoryBank[A].Timestamp < MemoryBank[B].Timestamp;
    });

    int32 NumToRemove = ShortTermIndices.Num() - MaxShortTermCount;
    TArray<int32> RemovalIndices;
    for (int32 i = 0; i < NumToRemove; ++i)
    {
        RemovalIndices.Add(ShortTermIndices[i]);
    }

    RemovalIndices.Sort([](const int32& A, const int32& B) { return A > B; });
    for (int32 Index : RemovalIndices)
    {
        MemoryBank.RemoveAt(Index);
    }
}

TArray<FWAIMemoryRecord> UWAIMemoryComponent::GetMemoriesByType(EWAIMemoryType Type) const
{
    TArray<FWAIMemoryRecord> Filtered;
    for (const FWAIMemoryRecord& Record : MemoryBank)
    {
        if (Record.MemoryType == Type)
        {
            Filtered.Add(Record);
        }
    }
    return Filtered;
}

FWAIMemoryRecord UWAIMemoryComponent::GetMostRecentMemory() const
{
    if (MemoryBank.Num() > 0)
    {
        return MemoryBank.Last();
    }
    return FWAIMemoryRecord();
}
