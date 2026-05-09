#include "WanaSandboxManager.h"

#include "WanaWorksCoreModule.h"
#include "GameFramework/Actor.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "Misc/Guid.h"

#if WITH_EDITOR
TMap<FString, TArray<uint8>> UWanaSandboxManager::BackupStore;
#endif

FString UWanaSandboxManager::BackupActorState(AActor* Target)
{
#if WITH_EDITOR
    if (!Target)
    {
        FWanaLogger::Warning(TEXT("BackupActorState called with null target."));
        return FString();
    }

    TArray<uint8> Bytes;
    FMemoryWriter MemoryWriter(Bytes, true);
    FObjectAndNameAsStringProxyArchive Ar(MemoryWriter, false);
    Ar.ArIsSaveGame = true;
    Target->Serialize(Ar);

    const FString BackupId = FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphens);
    BackupStore.Add(BackupId, MoveTemp(Bytes));

    FWanaLogger::Log(FString::Printf(TEXT("Backup created for %s with ID %s"), *Target->GetName(), *BackupId));
    return BackupId;
#else
    return FString();
#endif
}

TArray<FString> UWanaSandboxManager::ListBackups()
{
    TArray<FString> Ids;
#if WITH_EDITOR
    BackupStore.GetKeys(Ids);
#endif
    return Ids;
}

bool UWanaSandboxManager::RestoreFromBackup(const FString& BackupId, AActor* Target)
{
#if WITH_EDITOR
    if (!Target)
    {
        FWanaLogger::Warning(TEXT("RestoreFromBackup called with null target."));
        return false;
    }

    TArray<uint8>* Bytes = BackupStore.Find(BackupId);
    if (!Bytes)
    {
        FWanaLogger::Warning(FString::Printf(TEXT("No backup found with ID %s"), *BackupId));
        return false;
    }

    FMemoryReader MemoryReader(*Bytes, true);
    FObjectAndNameAsStringProxyArchive Ar(MemoryReader, false);
    Ar.ArIsSaveGame = true;
    Target->Serialize(Ar);

    FWanaLogger::Log(FString::Printf(TEXT("Restored backup %s onto %s"), *BackupId, *Target->GetName()));
    return true;
#else
    return false;
#endif
}

void UWanaSandboxManager::ClearBackups()
{
#if WITH_EDITOR
    BackupStore.Empty();
    FWanaLogger::Log(TEXT("Cleared all sandbox backups."));
#endif
}
