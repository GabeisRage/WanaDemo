#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "WanaMemorySubsystem.generated.h"

class FWanaMemoryBackend;

DECLARE_DYNAMIC_DELEGATE_TwoParams(FWanaMemoryReplyDelegate, bool, bSuccess, FString, Reply);

USTRUCT(BlueprintType)
struct WANAWORKSMEMORY_API FWanaRelationshipScores
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WanaWorks|Memory")
    float Trust = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WanaWorks|Memory")
    float Affinity = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WanaWorks|Memory")
    float Fear = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WanaWorks|Memory")
    float Respect = 0.5f;
};

UCLASS()
class WANAWORKSMEMORY_API UWanaMemorySubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual void BeginDestroy() override;

    UFUNCTION(BlueprintCallable, Category="WanaWorks|Memory")
    void TalkToCharacter(FString CharacterId, FString PlayerId, FString PlayerMessage, FWanaMemoryReplyDelegate OnReply);

    UFUNCTION(BlueprintCallable, Category="WanaWorks|Memory")
    bool SetIdentityState(FString CharacterId, FString StateKind, int32 SchemaVersion, FString JsonBlob, FString& OutError);

    UFUNCTION(BlueprintCallable, Category="WanaWorks|Memory")
    bool GetIdentityState(FString CharacterId, FString StateKind, FString& OutJsonBlob, int32& OutSchemaVersion, FString& OutError);

    UFUNCTION(BlueprintCallable, Category="WanaWorks|Memory")
    bool GetRelationshipScores(FString CharacterId, FString PlayerId, FWanaRelationshipScores& OutScores, FString& OutError);

    UFUNCTION(BlueprintCallable, Category="WanaWorks|Memory")
    bool SetRelationshipScores(FString CharacterId, FString PlayerId, FWanaRelationshipScores Scores, FString Reason, FString& OutError);

    UFUNCTION(BlueprintCallable, Category="WanaWorks|Memory")
    bool Remember(FString CharacterId, FString PlayerId, FString Content, float Importance, FString& OutError);

    UFUNCTION(BlueprintCallable, Category="WanaWorks|Memory")
    bool GetTranscript(FString CharacterId, FString PlayerId, int32 MaxTurns, FString& OutTranscript, FString& OutError);

    UFUNCTION(BlueprintCallable, Category="WanaWorks|Memory")
    bool GetRelationshipHistoryText(FString CharacterId, FString PlayerId, int32 MaxEvents, FString& OutHistory, FString& OutError);

    UFUNCTION(BlueprintCallable, Category="WanaWorks|Memory")
    bool SetActiveProvider(FString ProviderName, FString& OutError);

    UFUNCTION(BlueprintCallable, Category="WanaWorks|Memory")
    bool SetModelOverride(FString ModelName, FString& OutError);

    UFUNCTION(BlueprintCallable, Category="WanaWorks|Memory")
    bool ReloadProviderSettings(FString& OutError);

    UFUNCTION(BlueprintPure, Category="WanaWorks|Memory")
    bool IsDatabaseOpen() const;

    UFUNCTION(BlueprintPure, Category="WanaWorks|Memory")
    FString GetDatabasePath() const;

    UFUNCTION(BlueprintPure, Category="WanaWorks|Memory")
    FString GetActiveProviderId() const;

    UFUNCTION(BlueprintPure, Category="WanaWorks|Memory")
    FString GetLastStatus() const;

    UFUNCTION(BlueprintPure, Category="WanaWorks|Memory")
    FString GetProviderSummary() const;

private:
    void ShutdownBackend();
    bool EnsureReady(FString& OutError);
    void RebuildProvider();

    FWanaMemoryBackend* Backend = nullptr;
};

UCLASS()
class WANAWORKSMEMORY_API UWanaMemoryLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="WanaWorks|Memory", meta=(WorldContext="WorldContextObject"))
    static void TalkToCharacter(UObject* WorldContextObject, FString CharacterId, FString PlayerId, FString PlayerMessage, FWanaMemoryReplyDelegate OnReply);
};
