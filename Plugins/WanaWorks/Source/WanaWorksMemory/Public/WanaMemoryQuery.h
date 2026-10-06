#pragma once

#include "CoreMinimal.h"

/** One row in the editor timeline. Turns have no importance. */
struct WANAWORKSMEMORY_API FWanaMemoryTimelineItem
{
    FString Kind;
    FString Source;
    FString Timestamp;
    bool bHasImportance = false;
    float Importance = 0.f;
    FString Text;
    FString ProviderId;
    bool bPlayer = false;
    bool bAi = false;
};

struct WANAWORKSMEMORY_API FWanaMemoryEdge
{
    FString CharacterId;
    FString PlayerId;
    float Trust = 0.5f;
    float Affinity = 0.5f;
    float Fear = 0.f;
    float Respect = 0.5f;
    FString UpdatedAt;
};

struct WANAWORKSMEMORY_API FWanaMemoryHistoryPoint
{
    FString Timestamp;
    float Trust = 0.5f;
    float Affinity = 0.5f;
    float Fear = 0.f;
    float Respect = 0.5f;
    FString Reason;
    FString Source;
};

struct WANAWORKSMEMORY_API FWanaMemoryIdentityItem
{
    FString StateKind;
    int32 SchemaVersion = 0;
    FString JsonBlob;
    FString UpdatedAt;
};

/** Read-only view of Saved/WanaWorks/Memory/wana_memory.db for the editor tab.
    Does not create the file and does not run migrations. */
class WANAWORKSMEMORY_API FWanaMemoryDatabaseView
{
public:
    FWanaMemoryDatabaseView();
    ~FWanaMemoryDatabaseView();

    bool Open(const FString& Path, FString& OutError);
    void Close();
    bool IsOpen() const;

    bool ReadTimeline(const FString& CharacterId, const FString& PlayerId, TArray<FWanaMemoryTimelineItem>& OutItems, FString& OutError);
    bool ReadEdges(TArray<FWanaMemoryEdge>& OutEdges, FString& OutError);
    bool ReadHistory(const FString& CharacterId, const FString& PlayerId, TArray<FWanaMemoryHistoryPoint>& OutPoints, FString& OutError);
    bool ReadIdentities(const FString& CharacterId, TArray<FWanaMemoryIdentityItem>& OutStates, FString& OutError);

private:
    struct FImpl;
    TUniquePtr<FImpl> Impl;
};
