#include "WanaWorksCommandRegistry.h"

namespace
{
const TArray<FWanaCommandDefinition>& GetStaticCommandDefinitions()
{
    static const TArray<FWanaCommandDefinition> Definitions =
    {
        { FName(TEXT("help")), TEXT("help"), TEXT("Help"), TEXT("Display available commands."), TEXT("Status: Help displayed."), NAME_None, EWanaCommandAction::None, true, false },
        { FName(TEXT("clear")), TEXT("clear"), TEXT("Clear"), TEXT("Clear the output log."), TEXT("Status: Log cleared."), NAME_None, EWanaCommandAction::None, true, false },
        { FName(TEXT("echo")), TEXT("echo <text>"), TEXT("Echo"), TEXT("Write text to the output log."), TEXT("Status: Echo complete."), NAME_None, EWanaCommandAction::None, true, false },
        { FName(TEXT("weather_clear")), TEXT("weather clear"), TEXT("Clear"), TEXT("Apply the clear weather preset."), TEXT("Status: Applying clear weather."), FName(TEXT("weather")), EWanaCommandAction::WeatherClear, true, true },
        { FName(TEXT("weather_overcast")), TEXT("weather overcast"), TEXT("Overcast"), TEXT("Apply the overcast weather preset."), TEXT("Status: Applying overcast weather."), FName(TEXT("weather")), EWanaCommandAction::WeatherOvercast, true, true },
        { FName(TEXT("weather_storm")), TEXT("weather storm"), TEXT("Storm"), TEXT("Apply the storm weather preset."), TEXT("Status: Applying storm weather."), FName(TEXT("weather")), EWanaCommandAction::WeatherStorm, true, true },
        { FName(TEXT("spawn_cube")), TEXT("spawn cube"), TEXT("Spawn Cube"), TEXT("Spawn a cube actor in the editor world."), TEXT("Status: Spawning cube."), FName(TEXT("scene")), EWanaCommandAction::SpawnCube, true, true },
        { FName(TEXT("list_selection")), TEXT("list selection"), TEXT("List Selection"), TEXT("List selected actors in the editor."), TEXT("Status: Listing selection."), FName(TEXT("scene")), EWanaCommandAction::ListSelection, true, true },
        { FName(TEXT("classify_selected")), TEXT("classify selected"), TEXT("Classify Selected"), TEXT("Classify the selected actor."), TEXT("Status: Classifying selection."), FName(TEXT("wit")), EWanaCommandAction::ClassifySelected, true, true },
        { FName(TEXT("add_memory_test")), TEXT("add memory test"), TEXT("Add Memory Test"), TEXT("Add a test memory entry."), TEXT("Status: Adding memory test."), FName(TEXT("wai")), EWanaCommandAction::AddMemoryTest, true, true },
        { FName(TEXT("add_preference_test")), TEXT("add preference test"), TEXT("Add Preference Test"), TEXT("Add a test preference entry."), TEXT("Status: Adding preference test."), FName(TEXT("way")), EWanaCommandAction::AddPreferenceTest, true, true },
        { FName(TEXT("show_memory")), TEXT("show memory"), TEXT("Show Memory"), TEXT("Display current memory entries."), TEXT("Status: Showing memory."), FName(TEXT("wai")), EWanaCommandAction::ShowMemory, true, true },
        { FName(TEXT("show_preferences")), TEXT("show preferences"), TEXT("Show Preferences"), TEXT("Display current preference entries."), TEXT("Status: Showing preferences."), FName(TEXT("way")), EWanaCommandAction::ShowPreferences, true, true },
        { FName(TEXT("ensure_relationship_profile")), TEXT("ensure relationship profile"), TEXT("Ensure Relationship Profile"), TEXT("Create or reuse a relationship profile owned by the selected observer for the current target."), TEXT("Status: Ensuring relationship profile."), FName(TEXT("way_relationship")), EWanaCommandAction::EnsureRelationshipProfile, true, false },
        { FName(TEXT("show_relationship_profile")), TEXT("show relationship profile"), TEXT("Show Relationship Profile"), TEXT("Display the selected observer relationship profile for the current target."), TEXT("Status: Showing relationship profile."), FName(TEXT("way_relationship")), EWanaCommandAction::ShowRelationshipProfile, true, false },
        { FName(TEXT("apply_relationship_state")), TEXT("apply relationship state <state>"), TEXT("Apply Relationship State"), TEXT("Apply a relationship state to the selected observer profile for the current target."), TEXT("Status: Applying relationship state."), FName(TEXT("way_relationship")), EWanaCommandAction::ApplyRelationshipState, true, false }
    };

    return Definitions;
}
}

namespace WanaWorksCommandRegistry
{
const TArray<FWanaCommandDefinition>& GetCommandDefinitions()
{
    return GetStaticCommandDefinitions();
}

const FWanaCommandDefinition* FindCommandDefinitionById(FName CommandId)
{
    for (const FWanaCommandDefinition& Definition : GetStaticCommandDefinitions())
    {
        if (Definition.Id == CommandId)
        {
            return &Definition;
        }
    }

    return nullptr;
}

const FWanaCommandDefinition* FindCommandDefinitionByCommandText(const FString& CommandText)
{
    const FString TrimmedCommand = CommandText.TrimStartAndEnd();

    for (const FWanaCommandDefinition& Definition : GetStaticCommandDefinitions())
    {
        if (TrimmedCommand.Equals(Definition.CommandText, ESearchCase::IgnoreCase))
        {
            return &Definition;
        }
    }

    return nullptr;
}
}
