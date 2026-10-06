#include "WanaSettings.h"

#include "HAL/PlatformMisc.h"

namespace
{

FString ReadOpenAIKeyFromEnvironment()
{
    FString Key = FPlatformMisc::GetEnvironmentVariable(TEXT("WANA_OPENAI_API_KEY"));
    if (Key.IsEmpty())
    {
        Key = FPlatformMisc::GetEnvironmentVariable(TEXT("OPENAI_API_KEY"));
    }
    return Key;
}

} // namespace

UWanaSettings::UWanaSettings()
{
    // Available to existing readers of the property. Transient, so it is not
    // written back to DefaultEditor.ini.
    OpenAIApiKey = ReadOpenAIKeyFromEnvironment();
}

FString UWanaSettings::GetOpenAIApiKey()
{
    return ReadOpenAIKeyFromEnvironment();
}
