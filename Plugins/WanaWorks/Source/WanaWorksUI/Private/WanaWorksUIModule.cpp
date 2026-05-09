#include "WanaWorksUIModule.h"

#include "WanaWorksCoreModule.h"
#include "WanaWorksEditorTab.h"
#include "Modules/ModuleManager.h"
#include "Framework/Docking/TabManager.h"
#include "Widgets/Docking/SDockTab.h"
#include "Styling/AppStyle.h"

#define LOCTEXT_NAMESPACE "WanaWorksUI"

const FName FWanaWorksUIModule::WanaWorksTabName = FName(TEXT("WanaWorks"));

IMPLEMENT_MODULE(FWanaWorksUIModule, WanaWorksUI)

void FWanaWorksUIModule::StartupModule()
{
    FWanaLogger::Log(TEXT("WanaWorksUI startup — registering editor tab."));

    FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
        WanaWorksTabName,
        FOnSpawnTab::CreateRaw(this, &FWanaWorksUIModule::SpawnWanaWorksTab))
        .SetDisplayName(LOCTEXT("WanaWorksTabTitle", "WanaWorks"))
        .SetTooltipText(LOCTEXT("WanaWorksTabTooltip", "Open the WanaWorks control panel."))
        .SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"));
}

void FWanaWorksUIModule::ShutdownModule()
{
    if (FSlateApplication::IsInitialized())
    {
        FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(WanaWorksTabName);
    }
}

TSharedRef<SDockTab> FWanaWorksUIModule::SpawnWanaWorksTab(const FSpawnTabArgs& Args)
{
    return SNew(SDockTab)
        .TabRole(ETabRole::NomadTab)
        [
            SNew(SWanaWorksPanel)
        ];
}

#undef LOCTEXT_NAMESPACE
