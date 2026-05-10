#include "WanaProjectScanner.h"

#include "AIController.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

TArray<AActor*> UWanaProjectScanner::ScanForCharacterBlueprints(UObject* WorldContextObject)
{
    TArray<AActor*> Found;
    if (!WorldContextObject)
    {
        return Found;
    }
    UGameplayStatics::GetAllActorsOfClass(WorldContextObject, ACharacter::StaticClass(), Found);
    return Found;
}

TArray<AActor*> UWanaProjectScanner::ScanForAIPawns(UObject* WorldContextObject)
{
    TArray<AActor*> Found;
    if (!WorldContextObject)
    {
        return Found;
    }
    UGameplayStatics::GetAllActorsOfClass(WorldContextObject, APawn::StaticClass(), Found);
    return Found;
}

TArray<AActor*> UWanaProjectScanner::ScanForAIControllers(UObject* WorldContextObject)
{
    TArray<AActor*> Found;
    if (!WorldContextObject)
    {
        return Found;
    }
    UGameplayStatics::GetAllActorsOfClass(WorldContextObject, AAIController::StaticClass(), Found);
    return Found;
}

FString UWanaProjectScanner::GetScanSummary(UObject* WorldContextObject)
{
    const int32 NumCharacters = ScanForCharacterBlueprints(WorldContextObject).Num();
    const int32 NumPawns = ScanForAIPawns(WorldContextObject).Num();
    const int32 NumControllers = ScanForAIControllers(WorldContextObject).Num();
    return FString::Printf(TEXT("Characters: %d | AI Pawns: %d | AI Controllers: %d"),
        NumCharacters, NumPawns, NumControllers);
}
