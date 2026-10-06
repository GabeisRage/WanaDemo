#include "WanaMemoryConfig.h"

#include "WanaMemoryJson.h"
#include "WanaMemoryUtil.h"

#include <cstdlib>

namespace WanaMemory
{
namespace
{

bool IsSecretKey(const std::string& Key)
{
    const std::string Lower = ToLowerAscii(Key);
    static const char* Names[] = {
        "api_key",
        "apikey",
        "api-key",
        "openai_api_key",
        "xai_api_key",
        "grok_api_key",
        "authorization",
        "bearer",
        "access_token",
        "api_token"
    };
    for (const char* Name : Names)
    {
        if (Lower == Name)
        {
            return true;
        }
    }
    return false;
}

bool ContainsSecretKey(const Json::Value& Value)
{
    if (Value.Type == Json::Value::Kind::Object)
    {
        for (const std::pair<std::string, Json::Value>& Pair : Value.Object)
        {
            if (IsSecretKey(Pair.first) || ContainsSecretKey(Pair.second))
            {
                return true;
            }
        }
    }
    else if (Value.Type == Json::Value::Kind::Array)
    {
        for (const Json::Value& Child : Value.Array)
        {
            if (ContainsSecretKey(Child))
            {
                return true;
            }
        }
    }
    return false;
}

int ReadInt(const Json::Value& Root, const char* Key, int Fallback, int Lo, int Hi, std::string& Warning)
{
    const Json::Value* Child = Root.Find(Key);
    if (!Child)
    {
        return Fallback;
    }
    if (Child->Type != Json::Value::Kind::Number)
    {
        Warning += std::string(Key) + " was ignored because it was not a number. ";
        return Fallback;
    }
    return static_cast<int>(ClampRange(Child->Number, static_cast<double>(Lo), static_cast<double>(Hi)));
}

double ReadDouble(const Json::Value& Root, const char* Key, double Fallback, double Lo, double Hi, std::string& Warning)
{
    const Json::Value* Child = Root.Find(Key);
    if (!Child)
    {
        return Fallback;
    }
    if (Child->Type != Json::Value::Kind::Number)
    {
        Warning += std::string(Key) + " was ignored because it was not a number. ";
        return Fallback;
    }
    return ClampRange(Child->Number, Lo, Hi);
}

std::string SecretFromJson(const std::string& SecretsJson, const char* Field, std::string& Warning)
{
    if (Trim(SecretsJson).empty())
    {
        return std::string();
    }
    Json::Value Root;
    std::string Error;
    if (!Json::Parse(SecretsJson, Root, Error) || Root.Type != Json::Value::Kind::Object)
    {
        Warning += "provider.secrets.json was ignored because it is not a JSON object. ";
        return std::string();
    }
    const Json::Value* Child = Root.Find(Field);
    if (!Child)
    {
        return std::string();
    }
    if (Child->Type != Json::Value::Kind::String)
    {
        Warning += std::string(Field) + " in the secrets file was ignored because it was not a string. ";
        return std::string();
    }
    return Trim(Child->String);
}

bool UrlLooksLikeItCarriesAKey(const std::string& Url)
{
    const std::string Lower = ToLowerAscii(Url);
    return Lower.find("api_key=") != std::string::npos
        || Lower.find("apikey=") != std::string::npos
        || Lower.find("access_token=") != std::string::npos;
}

} // namespace

std::string ProcessEnvLookup::Get(const std::string& Name) const
{
    const char* Value = std::getenv(Name.c_str());
    return Value ? std::string(Value) : std::string();
}

Status LoadProviderSettings(const std::string& JsonText, ProviderSettings& OutSettings)
{
    OutSettings = ProviderSettings();
    if (Trim(JsonText).empty())
    {
        return Status::Ok();
    }
    Json::Value Root;
    std::string Error;
    if (!Json::Parse(JsonText, Root, Error) || Root.Type != Json::Value::Kind::Object)
    {
        return Status::Fail(Error.empty() ? "provider.json is not a JSON object" : Error);
    }
    if (ContainsSecretKey(Root))
    {
        OutSettings.bIgnoredEmbeddedSecret = true;
        OutSettings.Warning += "API key fields in provider.json were ignored. Put keys in the environment or in Saved/WanaWorks/Memory/provider.secrets.json. ";
    }

    const std::string Provider = ToLowerAscii(Trim(Root.GetString("provider", "grok")));
    if (Provider == "grok" || Provider == "xai")
    {
        OutSettings.Provider = "grok";
    }
    else if (Provider == "openai" || Provider == "chatgpt")
    {
        OutSettings.Provider = "openai";
    }
    else
    {
        return Status::Fail("provider must be grok or openai");
    }

    const std::string GrokModel = Trim(Root.GetString("grok_model"));
    const std::string OpenAIModel = Trim(Root.GetString("openai_model"));
    const std::string ModelOverride = Trim(Root.GetString("model_override"));
    if (!GrokModel.empty())
    {
        OutSettings.GrokModel = GrokModel;
    }
    if (!OpenAIModel.empty())
    {
        OutSettings.OpenAIModel = OpenAIModel;
    }
    OutSettings.ModelOverride = ModelOverride;

    const std::string GrokEndpoint = Trim(Root.GetString("grok_endpoint"));
    const std::string OpenAIEndpoint = Trim(Root.GetString("openai_endpoint"));
    const std::string EndpointOverride = Trim(Root.GetString("endpoint_override"));
    if (!GrokEndpoint.empty())
    {
        OutSettings.GrokEndpoint = GrokEndpoint;
    }
    if (!OpenAIEndpoint.empty())
    {
        OutSettings.OpenAIEndpoint = OpenAIEndpoint;
    }
    OutSettings.EndpointOverride = EndpointOverride;

    OutSettings.MaxContextChars = ReadInt(Root, "max_context_chars", OutSettings.MaxContextChars, 256, 100000, OutSettings.Warning);
    OutSettings.Temperature = ReadDouble(Root, "temperature", OutSettings.Temperature, 0.0, 2.0, OutSettings.Warning);
    OutSettings.MaxOutputTokens = ReadInt(Root, "max_output_tokens", OutSettings.MaxOutputTokens, 1, 4096, OutSettings.Warning);
    OutSettings.RecentTurnLimit = ReadInt(Root, "recent_turn_limit", OutSettings.RecentTurnLimit, 1, 100, OutSettings.Warning);
    OutSettings.TopKMemories = ReadInt(Root, "top_k_memories", OutSettings.TopKMemories, 1, 50, OutSettings.Warning);

    const std::string Embeddings = ToLowerAscii(Trim(Root.GetString("embeddings", "off")));
    if (Embeddings.empty() || Embeddings == "off" || Embeddings == "none")
    {
        OutSettings.Embeddings = "off";
    }
    else if (Embeddings == "local-hash")
    {
        OutSettings.Embeddings = "local-hash";
    }
    else
    {
        OutSettings.Embeddings = "off";
        OutSettings.Warning += "Unknown embeddings value was ignored. Use off or local-hash. ";
    }
    return Status::Ok();
}

std::string WriteProviderSettingsJson(const ProviderSettings& Settings)
{
    Json::Value Root = Json::MakeObject();
    Root.Set("provider", Json::MakeString(Settings.Provider));
    Root.Set("grok_model", Json::MakeString(Settings.GrokModel));
    Root.Set("openai_model", Json::MakeString(Settings.OpenAIModel));
    if (!Settings.ModelOverride.empty())
    {
        Root.Set("model_override", Json::MakeString(Settings.ModelOverride));
    }
    Root.Set("max_context_chars", Json::MakeNumber(static_cast<double>(Settings.MaxContextChars)));
    Root.Set("temperature", Json::MakeNumber(Settings.Temperature));
    Root.Set("max_output_tokens", Json::MakeNumber(static_cast<double>(Settings.MaxOutputTokens)));
    Root.Set("recent_turn_limit", Json::MakeNumber(static_cast<double>(Settings.RecentTurnLimit)));
    Root.Set("top_k_memories", Json::MakeNumber(static_cast<double>(Settings.TopKMemories)));
    Root.Set("embeddings", Json::MakeString(Settings.Embeddings));
    if (!Settings.EndpointOverride.empty())
    {
        Root.Set("endpoint_override", Json::MakeString(Settings.EndpointOverride));
    }
    return Json::Stringify(Root);
}

std::string CanonicalProviderId(const ProviderSettings& Settings)
{
    return Settings.Provider == "openai" ? std::string("openai") : std::string("grok");
}

std::string ActiveModel(const ProviderSettings& Settings)
{
    if (!Trim(Settings.ModelOverride).empty())
    {
        return Settings.ModelOverride;
    }
    return CanonicalProviderId(Settings) == "openai" ? Settings.OpenAIModel : Settings.GrokModel;
}

Status ActiveEndpoint(const ProviderSettings& Settings, std::string& OutEndpoint)
{
    OutEndpoint = Trim(Settings.EndpointOverride);
    if (OutEndpoint.empty())
    {
        OutEndpoint = CanonicalProviderId(Settings) == "openai" ? Settings.OpenAIEndpoint : Settings.GrokEndpoint;
    }
    if (UrlLooksLikeItCarriesAKey(OutEndpoint))
    {
        OutEndpoint.clear();
        return Status::Fail("the provider URL must not contain an API key");
    }
    if (OutEndpoint.find("://") == std::string::npos)
    {
        OutEndpoint.clear();
        return Status::Fail("the provider URL is not valid");
    }
    return Status::Ok();
}

std::string ResolveApiKey(
    const ProviderSettings& Settings,
    const IEnvLookup& Env,
    const std::string& SecretsJson,
    std::string& OutWarning)
{
    OutWarning.clear();
    const bool bOpenAI = CanonicalProviderId(Settings) == "openai";
    const char* Primary = bOpenAI ? "WANA_OPENAI_API_KEY" : "WANA_XAI_API_KEY";
    const char* Secondary = bOpenAI ? "OPENAI_API_KEY" : "XAI_API_KEY";
    const char* SecretField = bOpenAI ? "openai_api_key" : "xai_api_key";
    std::string Key = Trim(Env.Get(Primary));
    if (Key.empty())
    {
        Key = Trim(Env.Get(Secondary));
    }
    if (Key.empty())
    {
        Key = SecretFromJson(SecretsJson, SecretField, OutWarning);
    }
    return Key;
}

} // namespace WanaMemory
