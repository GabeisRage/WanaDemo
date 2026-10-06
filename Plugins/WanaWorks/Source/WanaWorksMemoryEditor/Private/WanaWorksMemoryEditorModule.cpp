#include "WanaWorksMemoryEditorModule.h"

#include "SWanaMemoryPanel.h"
#include "WanaMemoryEditorStyle.h"

#include "Framework/Application/SlateApplication.h"
#include "Framework/Docking/TabManager.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"

#define LOCTEXT_NAMESPACE "WanaWorksMemoryEditor"

const FName FWanaWorksMemoryEditorModule::TabId(TEXT("WanaWorksMemory"));

void FWanaWorksMemoryEditorModule::StartupModule()
{
    FWanaMemoryEditorStyle::Initialize();

    FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
        TabId,
        FOnSpawnTab::CreateRaw(this, &FWanaWorksMemoryEditorModule::SpawnTab))
        .SetDisplayName(LOCTEXT("TabTitle", "WanaWorks Memory"))
        .SetTooltipText(LOCTEXT("TabTooltip", "Inspect studio-owned character memory, relationships, and WIT/WAY/WAI/WAMI state."))
        .SetGroup(WorkspaceMenu::GetMenuStructure().GetToolsCategory())
        .SetMenuType(ETabSpawnerMenuType::Enabled);

    UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FWanaWorksMemoryEditorModule::RegisterMenus));
}

void FWanaWorksMemoryEditorModule::ShutdownModule()
{
    if (UToolMenus::IsToolMenuUIEnabled())
    {
        UToolMenus::UnRegisterStartupCallback(this);
        UToolMenus::UnregisterOwner(this);
    }
    if (FSlateApplication::IsInitialized())
    {
        FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(TabId);
    }
    FWanaMemoryEditorStyle::Shutdown();
}

void FWanaWorksMemoryEditorModule::RegisterMenus()
{
    FToolMenuOwnerScoped Owner(this);
    if (UToolMenu* Menu = UToolMenus::Get()->ExtendMenu(TEXT("LevelEditor.MainMenu.Tools")))
    {
        FToolMenuSection& Section = Menu->FindOrAddSection(TEXT("WanaWorksMemory"));
        Section.AddMenuEntry(
            TEXT("OpenWanaWorksMemory"),
            LOCTEXT("OpenLabel", "WanaWorks Memory"),
            LOCTEXT("OpenTooltip", "Open the memory timeline, relationship graph, and state inspector."),
            FSlateIcon(),
            FUIAction(FExecuteAction::CreateRaw(this, &FWanaWorksMemoryEditorModule::OpenTab)));
    }
}

TSharedRef<SDockTab> FWanaWorksMemoryEditorModule::SpawnTab(const FSpawnTabArgs& Args)
{
    (void)Args;
    return SNew(SDockTab)
        .TabRole(ETabRole::NomadTab)
        .Label(LOCTEXT("DockLabel", "WanaWorks Memory"))
        [
            SNew(SWanaMemoryPanel)
        ];
}

void FWanaWorksMemoryEditorModule::OpenTab()
{
    FGlobalTabmanager::Get()->TryInvokeTab(TabId);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FWanaWorksMemoryEditorModule, WanaWorksMemoryEditor)
