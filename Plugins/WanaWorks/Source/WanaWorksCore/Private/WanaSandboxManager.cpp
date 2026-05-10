#include "WanaSandboxManager.h"

#if WITH_EDITOR
#include "GameFramework/Actor.h"
#include "Misc/Guid.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "WanaWorksCoreModule.h"

TMap<FString, TArray<uint8>> UWanaSandboxManager::BackupStore;

FString UWanaSandboxManager::BackupActorState(AActor* Target)
{
    if (!Target)
    {
        FWanaLogger::Warning(TEXT("WanaSandboxManager::BackupActorState called with null target."));
        return FString();
    }

    TArray<uint8> Bytes;
    FMemoryWriter MemoryWriter(Bytes, true);
    FObjectAndNameAsStringProxyArchive Archive(MemoryWriter, false);
    Target->Serialize(Archive);

    const FString BackupId = FGuid::NewGuid().ToString();
    BackupStore.Add(BackupId, MoveTemp(Bytes));
    FWanaLogger::Log(FString::Printf(TEXT("WanaSandboxManager: Backed up %s as %s"), *Target->GetName(), *BackupId));
    return BackupId;
}

TArray<FString> UWanaSandboxManager::ListBackups()
{
    TArray<FString> Keys;
    BackupStore.GetKeys(Keys);
    return Keys;
}

bool UWanaSandboxManager::RestoreFromBackup(const FString& BackupId, AActor* Target)
{
    if (!Target)
    {
        FWanaLogger::Warning(TEXT("WanaSandboxManager::RestoreFromBackup called with null target."));
        return false;
    }

    TArray<uint8>* Bytes = BackupStore.Find(BackupId);
    if (!Bytes)
    {
        FWanaLogger::Warning(FString::Printf(TEXT("WanaSandboxManager: Backup ID %s not found."), *BackupId));
        return false;
    }

    FMemoryReader MemoryReader(*Bytes, true);
    FObjectAndNameAsStringProxyArchive Archive(MemoryReader, false);
    Target->Serialize(Archive);
    FWanaLogger::Log(FString::Printf(TEXT("WanaSandboxManager: Restored %s from %s"), *Target->GetName(), *BackupId));
    return true;
}
#endif
