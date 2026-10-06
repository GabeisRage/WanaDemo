#pragma once

#include "Modules/ModuleManager.h"

class WANAWORKSMEMORYEDITOR_API FWanaWorksMemoryEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
    void RegisterMenus();
    TSharedRef<class SDockTab> SpawnTab(const class FSpawnTabArgs& Args);
    void OpenTab();

    static const FName TabId;
};
