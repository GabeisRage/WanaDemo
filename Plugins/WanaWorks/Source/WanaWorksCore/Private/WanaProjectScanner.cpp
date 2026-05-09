#include "WanaProjectScanner.h"

#include "WanaWorksCoreModule.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "AIController.h"
#include "Kismet/GameplayStatics.h"

TArray<AActor*> UWanaProjectScanner::ScanForCharacterBlueprints(UObject* WorldContext)
{
    TArray<AActor*> Results;
    if (!WorldContext)
    {
        return Results;
    }

    UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::LogAndReturnNull) : nullptr;
    if (!World)
    {
        return Results;
    }

    UGameplayStatics::GetAllActorsOfClass(World, ACharacter::StaticClass(), Results);
    FWanaLogger::Log(FString::Printf(TEXT("ScanForCharacterBlueprints found %d actors."), Results.Num()));
    return Results;
}

TArray<AActor*> UWanaProjectScanner::ScanForAIPawns(UObject* WorldContext)
{
    TArray<AActor*> Results;
    if (!WorldContext)
    {
        return Results;
    }

    UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::LogAndReturnNull) : nullptr;
    if (!World)
    {
        return Results;
    }

    TArray<AActor*> AllPawns;
    UGameplayStatics::GetAllActorsOfClass(World, APawn::StaticClass(), AllPawns);
    for (AActor* Actor : AllPawns)
    {
        APawn* Pawn = Cast<APawn>(Actor);
        if (Pawn && Pawn->IsBotControlled())
        {
            Results.Add(Actor);
        }
    }

    FWanaLogger::Log(FString::Printf(TEXT("ScanForAIPawns found %d AI pawns."), Results.Num()));
    return Results;
}

TArray<AActor*> UWanaProjectScanner::ScanForAIControllers(UObject* WorldContext)
{
    TArray<AActor*> Results;
    if (!WorldContext)
    {
        return Results;
    }

    UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::LogAndReturnNull) : nullptr;
    if (!World)
    {
        return Results;
    }

    UGameplayStatics::GetAllActorsOfClass(World, AAIController::StaticClass(), Results);
    FWanaLogger::Log(FString::Printf(TEXT("ScanForAIControllers found %d controllers."), Results.Num()));
    return Results;
}

FString UWanaProjectScanner::GetScanSummary(UObject* WorldContext)
{
    const int32 Characters = ScanForCharacterBlueprints(WorldContext).Num();
    const int32 AIPawns = ScanForAIPawns(WorldContext).Num();
    const int32 AIControllers = ScanForAIControllers(WorldContext).Num();

    return FString::Printf(
        TEXT("WanaWorks Project Scan Summary:\n  Character actors: %d\n  AI Pawns: %d\n  AI Controllers: %d"),
        Characters, AIPawns, AIControllers);
}
