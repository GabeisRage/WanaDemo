#pragma once

#include "CoreMinimal.h"
#include "WanaWorksCommandDispatcher.h"

struct WANAWORKSCORE_API FWanaCommandDefinition
{
    FName Id;
    const TCHAR* CommandText = TEXT("");
    const TCHAR* DisplayLabel = TEXT("");
    const TCHAR* Description = TEXT("");
    const TCHAR* StatusMessage = TEXT("");
    FName SectionId;
    EWanaCommandAction Action = EWanaCommandAction::None;
    bool bShowInHelp = true;
    bool bShowAsButton = false;
};

namespace WanaWorksCommandRegistry
{
    WANAWORKSCORE_API const TArray<FWanaCommandDefinition>& GetCommandDefinitions();
    WANAWORKSCORE_API const FWanaCommandDefinition* FindCommandDefinitionById(FName CommandId);
    WANAWORKSCORE_API const FWanaCommandDefinition* FindCommandDefinitionByCommandText(const FString& CommandText);
}
