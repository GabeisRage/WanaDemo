#pragma once

#include "WanaMemoryStatus.h"

#include <string>

namespace WanaMemory
{

struct ProviderSettings
{
    std::string Provider = "grok";
    std::string GrokModel = "grok-4";
    std::string OpenAIModel = "gpt-4o-mini";
    std::string ModelOverride;
    std::string GrokEndpoint = "https://api.x.ai/v1/chat/completions";
    std::string OpenAIEndpoint = "https://api.openai.com/v1/chat/completions";
    std::string EndpointOverride;
    int MaxContextChars = 8000;
    double Temperature = 0.7;
    int MaxOutputTokens = 400;
    int RecentTurnLimit = 16;
    int TopKMemories = 6;
    std::string Embeddings = "off";
    bool bIgnoredEmbeddedSecret = false;
    std::string Warning;
};

class IEnvLookup
{
public:
    virtual ~IEnvLookup() = default;
    virtual std::string Get(const std::string& Name) const = 0;
};

class ProcessEnvLookup final : public IEnvLookup
{
public:
    std::string Get(const std::string& Name) const override;
};

/* Loads provider selection. Key fields in this JSON are ignored on purpose. */
Status LoadProviderSettings(const std::string& JsonText, ProviderSettings& OutSettings);
std::string WriteProviderSettingsJson(const ProviderSettings& Settings);
std::string CanonicalProviderId(const ProviderSettings& Settings);
std::string ActiveModel(const ProviderSettings& Settings);
Status ActiveEndpoint(const ProviderSettings& Settings, std::string& OutEndpoint);

/* Env vars win. The secrets JSON is the gitignored fallback, never provider.json. */
std::string ResolveApiKey(
    const ProviderSettings& Settings,
    const IEnvLookup& Env,
    const std::string& SecretsJson,
    std::string& OutWarning);

} // namespace WanaMemory
