#include "WanaMemorySubsystem.h"

#include "WanaWorksMemoryModule.h"

#include "Async/Async.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Kismet/GameplayStatics.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#include "WanaEmbedding.h"
#include "WanaLLMProvider.h"
#include "WanaMemoryConfig.h"
#include "WanaMemoryOrchestrator.h"
#include "WanaMemoryPrompt.h"
#include "WanaMemoryUtil.h"

#include <atomic>
#include <memory>

namespace
{

std::string ToUtf8(const FString& Text)
{
    return std::string(TCHAR_TO_UTF8(*Text));
}

FString FromUtf8(const std::string& Text)
{
    return FString(UTF8_TO_TCHAR(Text.c_str()));
}

class FWanaUeEnvLookup final : public WanaMemory::IEnvLookup
{
public:
    std::string Get(const std::string& Name) const override
    {
        return ToUtf8(FPlatformMisc::GetEnvironmentVariable(UTF8_TO_TCHAR(Name.c_str())));
    }
};

class FWanaHttpLLMProvider final : public WanaMemory::ILLMProvider
{
public:
    FWanaHttpLLMProvider(FString InProviderId, FString InEndpoint, TFunction<FString()> InKeyGetter)
        : ProviderName(MoveTemp(InProviderId))
        , Endpoint(MoveTemp(InEndpoint))
        , KeyGetter(MoveTemp(InKeyGetter))
    {
    }

    std::string ProviderId() const override
    {
        return ToUtf8(ProviderName);
    }

    void CompleteAsync(const WanaMemory::LLMRequest& Request, WanaMemory::ResultCallback<WanaMemory::LLMResult> Callback) override
    {
        const std::string ProviderUtf8 = ToUtf8(ProviderName);
        const FString Key = KeyGetter ? KeyGetter() : FString();
        if (Key.IsEmpty())
        {
            const TCHAR* Hint = ProviderName == TEXT("openai")
                ? TEXT("No OpenAI API key is set. Use WANA_OPENAI_API_KEY, OPENAI_API_KEY, or Saved/WanaWorks/Memory/provider.secrets.json. Do not put the key in UWanaSettings or DefaultEditor.ini.")
                : TEXT("No xAI API key is set. Use WANA_XAI_API_KEY, XAI_API_KEY, or Saved/WanaWorks/Memory/provider.secrets.json. Do not put the key in UWanaSettings or DefaultEditor.ini.");
            Callback(WanaMemory::LLMResult::Fail(ToUtf8(FString(Hint)), ProviderUtf8));
            return;
        }

        struct FOnce
        {
            WanaMemory::ResultCallback<WanaMemory::LLMResult> Callback;
            std::atomic<bool> bFired{false};
        };
        TSharedRef<FOnce, ESPMode::ThreadSafe> Once = MakeShared<FOnce, ESPMode::ThreadSafe>();
        Once->Callback = std::move(Callback);

        const FString ProviderCopy = ProviderName;
        auto Fire = [Once, ProviderCopy](WanaMemory::LLMResult Result)
        {
            bool bExpected = false;
            if (!Once->bFired.compare_exchange_strong(bExpected, true))
            {
                return;
            }
            if (Result.ProviderId.empty())
            {
                Result.ProviderId = ToUtf8(ProviderCopy);
            }
            Once->Callback(Result);
        };

        const std::string Body = WanaMemory::BuildChatCompletionRequestJson(Request);
        TArray<uint8> Bytes;
        Bytes.Append(reinterpret_cast<const uint8*>(Body.data()), static_cast<int32>(Body.size()));

        TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
        HttpRequest->SetURL(Endpoint);
        HttpRequest->SetVerb(TEXT("POST"));
        HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
        HttpRequest->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *Key));
        HttpRequest->SetTimeout(30.0f);
        HttpRequest->SetContent(Bytes);
        HttpRequest->OnProcessRequestComplete().BindLambda([Fire](FHttpRequestPtr /*Request*/, FHttpResponsePtr Response, bool bConnected)
        {
            if (!bConnected || !Response.IsValid())
            {
                Fire(WanaMemory::LLMResult::Fail("The language model request did not complete."));
                return;
            }
            std::string Content;
            std::string Error;
            if (!WanaMemory::ParseChatCompletionBody(ToUtf8(Response->GetContentAsString()), Response->GetResponseCode(), Content, Error))
            {
                Fire(WanaMemory::LLMResult::Fail(Error.empty() ? "The language model request failed." : Error));
                return;
            }
            Fire(WanaMemory::LLMResult::Success(std::move(Content), std::string()));
        });

        // ProcessRequest returns immediately. Do not call ProcessRequestUntilComplete.
        if (!HttpRequest->ProcessRequest())
        {
            Fire(WanaMemory::LLMResult::Fail("The language model request could not be started."));
        }
    }

private:
    FString ProviderName;
    FString Endpoint;
    TFunction<FString()> KeyGetter;
};

} // namespace

class FWanaMemoryBackend
{
public:
    std::shared_ptr<WanaMemory::MemoryStore> Store = std::make_shared<WanaMemory::MemoryStore>();
    std::shared_ptr<WanaMemory::ILLMProvider> Provider;
    std::shared_ptr<WanaMemory::IEmbeddingProvider> Embeddings;
    WanaMemory::ProviderSettings Settings;
    FString DatabasePath;
    FString ConfigPath;
    FString SecretsPath;
    FString LastStatus;
    bool bOpen = false;
    bool bKeyPresent = false;
    std::shared_ptr<WanaMemory::IMemoryWriteListener> WriteListener;
};

class FWanaSubsystemWriteListener final : public WanaMemory::IMemoryWriteListener
{
public:
    TWeakObjectPtr<UWanaMemorySubsystem> Owner;

    void OnMemoryWrite(const WanaMemory::MemoryWriteNotice& Notice) override
    {
        const FString CharacterId = FromUtf8(Notice.CharacterId);
        const FString PlayerId = FromUtf8(Notice.PlayerId);
        const FString Kind = FromUtf8(Notice.Kind);
        TWeakObjectPtr<UWanaMemorySubsystem> WeakOwner = Owner;
        auto Fire = [WeakOwner, CharacterId, PlayerId, Kind]()
        {
            if (UWanaMemorySubsystem* Subsystem = WeakOwner.Get())
            {
                Subsystem->BroadcastWrite(CharacterId, PlayerId, Kind);
            }
        };
        if (IsInGameThread())
        {
            Fire();
        }
        else
        {
            AsyncTask(ENamedThreads::GameThread, MoveTemp(Fire));
        }
    }
};

void UWanaMemorySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    Backend = new FWanaMemoryBackend();
    std::shared_ptr<FWanaSubsystemWriteListener> Listener = std::make_shared<FWanaSubsystemWriteListener>();
    Listener->Owner = this;
    Backend->WriteListener = Listener;
    const FString Directory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("WanaWorks"), TEXT("Memory"));
    IFileManager::Get().MakeDirectory(*Directory, true);
    Backend->DatabasePath = FPaths::Combine(Directory, TEXT("wana_memory.db"));
    Backend->ConfigPath = FPaths::Combine(Directory, TEXT("provider.json"));
    Backend->SecretsPath = FPaths::Combine(Directory, TEXT("provider.secrets.json"));

    const WanaMemory::Status Opened = Backend->Store->Open(ToUtf8(Backend->DatabasePath));
    Backend->bOpen = Opened.bOk;
    if (!Opened.bOk)
    {
        Backend->LastStatus = FromUtf8(Opened.Message);
        UE_LOG(LogWanaMemory, Warning, TEXT("Memory database did not open: %s"), *Backend->LastStatus);
        return;
    }

    FString Error;
    if (!ReloadProviderSettings(Error))
    {
        Backend->LastStatus = Error;
        UE_LOG(LogWanaMemory, Warning, TEXT("Memory provider settings were not loaded: %s"), *Error);
    }
}

void UWanaMemorySubsystem::Deinitialize()
{
    ShutdownBackend();
    Super::Deinitialize();
}

void UWanaMemorySubsystem::BeginDestroy()
{
    ShutdownBackend();
    Super::BeginDestroy();
}

void UWanaMemorySubsystem::ShutdownBackend()
{
    if (!Backend)
    {
        return;
    }
    if (Backend->Store)
    {
        Backend->Store->Close();
    }
    delete Backend;
    Backend = nullptr;
}

bool UWanaMemorySubsystem::EnsureReady(FString& OutError)
{
    if (!Backend || !Backend->bOpen || !Backend->Store || !Backend->Store->IsOpen())
    {
        OutError = TEXT("The memory database is not open.");
        return false;
    }
    OutError.Reset();
    return true;
}

void UWanaMemorySubsystem::RebuildProvider()
{
    if (!Backend)
    {
        return;
    }
    std::string Endpoint;
    const WanaMemory::Status EndpointStatus = WanaMemory::ActiveEndpoint(Backend->Settings, Endpoint);
    if (!EndpointStatus.bOk)
    {
        Backend->Provider.reset();
        Backend->LastStatus = FromUtf8(EndpointStatus.Message);
        return;
    }

    const FString ProviderId = FromUtf8(WanaMemory::CanonicalProviderId(Backend->Settings));
    const FString SecretsPath = Backend->SecretsPath;
    const bool bOpenAI = ProviderId == TEXT("openai");
    TFunction<FString()> KeyGetter = [SecretsPath, bOpenAI]()
    {
        FWanaUeEnvLookup Env;
        WanaMemory::ProviderSettings Settings;
        Settings.Provider = bOpenAI ? "openai" : "grok";
        FString SecretsText;
        FFileHelper::LoadFileToString(SecretsText, *SecretsPath);
        std::string Warning;
        return FromUtf8(WanaMemory::ResolveApiKey(Settings, Env, ToUtf8(SecretsText), Warning));
    };
    Backend->bKeyPresent = !KeyGetter().IsEmpty();
    Backend->Provider = std::make_shared<FWanaHttpLLMProvider>(ProviderId, FromUtf8(Endpoint), KeyGetter);
    if (Backend->Settings.Embeddings == "local-hash")
    {
        Backend->Embeddings = std::make_shared<WanaMemory::LocalHashEmbedding>();
    }
    else
    {
        Backend->Embeddings.reset();
    }
}

bool UWanaMemorySubsystem::ReloadProviderSettings(FString& OutError)
{
    if (!Backend)
    {
        OutError = TEXT("Memory is not initialized.");
        return false;
    }
    FString JsonText;
    if (FPaths::FileExists(Backend->ConfigPath))
    {
        if (!FFileHelper::LoadFileToString(JsonText, *Backend->ConfigPath))
        {
            OutError = TEXT("provider.json could not be read.");
            return false;
        }
    }
    WanaMemory::ProviderSettings Settings;
    const WanaMemory::Status Loaded = WanaMemory::LoadProviderSettings(ToUtf8(JsonText), Settings);
    if (!Loaded.bOk)
    {
        OutError = FromUtf8(Loaded.Message);
        return false;
    }
    Backend->Settings = Settings;
    if (Settings.bIgnoredEmbeddedSecret)
    {
        UE_LOG(LogWanaMemory, Warning, TEXT("provider.json contained an API key field. It was ignored. Use an environment variable or Saved/WanaWorks/Memory/provider.secrets.json."));
    }
    if (!Settings.Warning.empty())
    {
        Backend->LastStatus = FromUtf8(Settings.Warning);
    }
    RebuildProvider();
    if (!Backend->Provider)
    {
        OutError = Backend->LastStatus.IsEmpty() ? FString(TEXT("The language model provider could not be created.")) : Backend->LastStatus;
        return false;
    }
    OutError.Reset();
    return true;
}

void UWanaMemorySubsystem::TalkToCharacter(FString CharacterId, FString PlayerId, FString PlayerMessage, FWanaMemoryReplyDelegate OnReply)
{
    FString Error;
    if (!EnsureReady(Error))
    {
        OnReply.ExecuteIfBound(false, Error);
        return;
    }
    if (!ReloadProviderSettings(Error))
    {
        OnReply.ExecuteIfBound(false, Error);
        return;
    }

    WanaMemory::TalkRequest Request;
    Request.CharacterId = ToUtf8(CharacterId);
    Request.PlayerId = ToUtf8(PlayerId);
    Request.PlayerMessage = ToUtf8(PlayerMessage);
    Request.Model = WanaMemory::ActiveModel(Backend->Settings);
    Request.Temperature = Backend->Settings.Temperature;
    Request.MaxOutputTokens = Backend->Settings.MaxOutputTokens;
    Request.MaxContextChars = Backend->Settings.MaxContextChars;
    Request.RecentTurnLimit = Backend->Settings.RecentTurnLimit;
    Request.TopKMemories = Backend->Settings.TopKMemories;
    Request.WriteListener = Backend->WriteListener;

    TWeakObjectPtr<UWanaMemorySubsystem> WeakThis(this);
    std::shared_ptr<WanaMemory::ILLMProvider> Provider = Backend->Provider;
    std::shared_ptr<WanaMemory::MemoryStore> Store = Backend->Store;
    std::shared_ptr<WanaMemory::IEmbeddingProvider> Embeddings = Backend->Embeddings;
    WanaMemory::TalkAsync(Store, std::move(Request), *Provider, Embeddings,
        WanaMemory::ResultCallback<WanaMemory::TalkResult>([WeakThis, OnReply](const WanaMemory::TalkResult& Result)
        {
            const WanaMemory::TalkResult Copy = Result;
            auto Fire = [WeakThis, OnReply, Copy]()
            {
                if (!WeakThis.IsValid() || WeakThis->Backend == nullptr)
                {
                    return;
                }
                if (Copy.Outcome.bOk)
                {
                    WeakThis->Backend->LastStatus = Copy.Warning.empty() ? FString(TEXT("Reply ready.")) : FromUtf8(Copy.Warning);
                    OnReply.ExecuteIfBound(true, FromUtf8(Copy.Reply));
                }
                else
                {
                    const FString Message = FromUtf8(Copy.Outcome.Message);
                    WeakThis->Backend->LastStatus = Message;
                    OnReply.ExecuteIfBound(false, Message);
                }
            };
            if (IsInGameThread())
            {
                Fire();
            }
            else
            {
                AsyncTask(ENamedThreads::GameThread, Fire);
            }
        }));
}

bool UWanaMemorySubsystem::SetIdentityState(FString CharacterId, FString StateKind, int32 SchemaVersion, FString JsonBlob, FString& OutError)
{
    if (!EnsureReady(OutError))
    {
        return false;
    }
    WanaMemory::IdentityState State;
    State.CharacterId = ToUtf8(CharacterId);
    State.StateKind = ToUtf8(StateKind);
    State.SchemaVersion = SchemaVersion;
    State.JsonBlob = ToUtf8(JsonBlob);
    const WanaMemory::Status Stored = Backend->Store->SetIdentity(State);
    if (!Stored.bOk)
    {
        OutError = FromUtf8(Stored.Message);
        Backend->LastStatus = OutError;
        return false;
    }
    OutError.Reset();
    BroadcastWrite(CharacterId, FString(), TEXT("identity"));
    return true;
}

bool UWanaMemorySubsystem::GetIdentityState(FString CharacterId, FString StateKind, FString& OutJsonBlob, int32& OutSchemaVersion, FString& OutError)
{
    OutJsonBlob.Reset();
    OutSchemaVersion = 0;
    if (!EnsureReady(OutError))
    {
        return false;
    }
    WanaMemory::IdentityState State;
    bool bFound = false;
    const WanaMemory::Status Loaded = Backend->Store->GetIdentity(ToUtf8(CharacterId), ToUtf8(StateKind), State, bFound);
    if (!Loaded.bOk)
    {
        OutError = FromUtf8(Loaded.Message);
        return false;
    }
    if (!bFound)
    {
        OutError = TEXT("No identity state is stored for that character and kind.");
        return false;
    }
    OutJsonBlob = FromUtf8(State.JsonBlob);
    OutSchemaVersion = State.SchemaVersion;
    OutError.Reset();
    return true;
}

bool UWanaMemorySubsystem::GetRelationshipScores(FString CharacterId, FString PlayerId, FWanaRelationshipScores& OutScores, FString& OutError)
{
    if (!EnsureReady(OutError))
    {
        return false;
    }
    WanaMemory::RelationshipScores Scores;
    bool bFound = false;
    const WanaMemory::Status Loaded = Backend->Store->GetRelationship(ToUtf8(CharacterId), ToUtf8(PlayerId), Scores, bFound);
    if (!Loaded.bOk)
    {
        OutError = FromUtf8(Loaded.Message);
        return false;
    }
    OutScores.Trust = static_cast<float>(Scores.Trust);
    OutScores.Affinity = static_cast<float>(Scores.Affinity);
    OutScores.Fear = static_cast<float>(Scores.Fear);
    OutScores.Respect = static_cast<float>(Scores.Respect);
    OutError.Reset();
    return true;
}

bool UWanaMemorySubsystem::SetRelationshipScores(FString CharacterId, FString PlayerId, FWanaRelationshipScores Scores, FString Reason, FString& OutError)
{
    if (!EnsureReady(OutError))
    {
        return false;
    }
    WanaMemory::RelationshipScores Native;
    Native.Trust = Scores.Trust;
    Native.Affinity = Scores.Affinity;
    Native.Fear = Scores.Fear;
    Native.Respect = Scores.Respect;
    bool bChanged = false;
    const WanaMemory::Status Stored = Backend->Store->SetRelationship(
        ToUtf8(CharacterId), ToUtf8(PlayerId), Native, ToUtf8(Reason), "manual", bChanged);
    if (!Stored.bOk)
    {
        OutError = FromUtf8(Stored.Message);
        return false;
    }
    OutError.Reset();
    BroadcastWrite(CharacterId, PlayerId, TEXT("relationship"));
    return true;
}

bool UWanaMemorySubsystem::Remember(FString CharacterId, FString PlayerId, FString Content, float Importance, FString& OutError)
{
    if (!EnsureReady(OutError))
    {
        return false;
    }
    WanaMemory::SalientMemory Memory;
    Memory.CharacterId = ToUtf8(CharacterId);
    Memory.PlayerId = ToUtf8(PlayerId);
    Memory.Content = ToUtf8(Content);
    Memory.Importance = Importance;
    Memory.Source = "manual";
    if (Backend->Embeddings)
    {
        std::vector<float> Vector;
        if (Backend->Embeddings->Embed(Memory.Content, Vector))
        {
            Memory.Embedding = std::move(Vector);
        }
    }
    std::int64_t Id = 0;
    const WanaMemory::Status Stored = Backend->Store->AddMemory(Memory, Id);
    if (!Stored.bOk)
    {
        OutError = FromUtf8(Stored.Message);
        return false;
    }
    OutError.Reset();
    BroadcastWrite(CharacterId, PlayerId, TEXT("memory"));
    return true;
}

bool UWanaMemorySubsystem::GetTranscript(FString CharacterId, FString PlayerId, int32 MaxTurns, FString& OutTranscript, FString& OutError)
{
    OutTranscript.Reset();
    if (!EnsureReady(OutError))
    {
        return false;
    }
    std::vector<WanaMemory::Turn> Turns;
    const WanaMemory::Status Loaded = Backend->Store->GetRecentTurns(ToUtf8(CharacterId), ToUtf8(PlayerId), MaxTurns, Turns);
    if (!Loaded.bOk)
    {
        OutError = FromUtf8(Loaded.Message);
        return false;
    }
    OutTranscript = FromUtf8(WanaMemory::FormatTranscript(Turns));
    OutError.Reset();
    return true;
}

bool UWanaMemorySubsystem::GetRelationshipHistoryText(FString CharacterId, FString PlayerId, int32 MaxEvents, FString& OutHistory, FString& OutError)
{
    OutHistory.Reset();
    if (!EnsureReady(OutError))
    {
        return false;
    }
    std::vector<WanaMemory::RelationshipEvent> Events;
    const WanaMemory::Status Loaded = Backend->Store->GetRelationshipHistory(ToUtf8(CharacterId), ToUtf8(PlayerId), MaxEvents, Events);
    if (!Loaded.bOk)
    {
        OutError = FromUtf8(Loaded.Message);
        return false;
    }
    OutHistory = FromUtf8(WanaMemory::FormatRelationshipHistory(Events));
    OutError.Reset();
    return true;
}

bool UWanaMemorySubsystem::SetActiveProvider(FString ProviderName, FString& OutError)
{
    if (!Backend)
    {
        OutError = TEXT("Memory is not initialized.");
        return false;
    }
    WanaMemory::ProviderSettings Settings = Backend->Settings;
    const std::string Requested = ToUtf8(ProviderName.TrimStartAndEnd());
    if (WanaMemory::EqualsIgnoreCase(Requested, "grok") || WanaMemory::EqualsIgnoreCase(Requested, "xai"))
    {
        Settings.Provider = "grok";
    }
    else if (WanaMemory::EqualsIgnoreCase(Requested, "openai") || WanaMemory::EqualsIgnoreCase(Requested, "chatgpt"))
    {
        Settings.Provider = "openai";
    }
    else
    {
        OutError = TEXT("Provider must be grok or openai.");
        return false;
    }
    const std::string Json = WanaMemory::WriteProviderSettingsJson(Settings);
    if (!FFileHelper::SaveStringToFile(FromUtf8(Json), *Backend->ConfigPath))
    {
        OutError = TEXT("Could not write Saved/WanaWorks/Memory/provider.json.");
        return false;
    }
    return ReloadProviderSettings(OutError);
}

bool UWanaMemorySubsystem::SetModelOverride(FString ModelName, FString& OutError)
{
    if (!Backend)
    {
        OutError = TEXT("Memory is not initialized.");
        return false;
    }
    WanaMemory::ProviderSettings Settings = Backend->Settings;
    Settings.ModelOverride = ToUtf8(ModelName.TrimStartAndEnd());
    const std::string Json = WanaMemory::WriteProviderSettingsJson(Settings);
    if (!FFileHelper::SaveStringToFile(FromUtf8(Json), *Backend->ConfigPath))
    {
        OutError = TEXT("Could not write Saved/WanaWorks/Memory/provider.json.");
        return false;
    }
    return ReloadProviderSettings(OutError);
}

bool UWanaMemorySubsystem::IsDatabaseOpen() const
{
    return Backend && Backend->bOpen && Backend->Store && Backend->Store->IsOpen();
}

FString UWanaMemorySubsystem::GetDatabasePath() const
{
    return Backend ? Backend->DatabasePath : FString();
}

FString UWanaMemorySubsystem::GetActiveProviderId() const
{
    return Backend ? FromUtf8(WanaMemory::CanonicalProviderId(Backend->Settings)) : FString();
}

FString UWanaMemorySubsystem::GetLastStatus() const
{
    return Backend ? Backend->LastStatus : FString();
}

FString UWanaMemorySubsystem::GetProviderSummary() const
{
    if (!Backend)
    {
        return TEXT("Memory is not initialized.");
    }
    return FString::Printf(
        TEXT("Provider %s, model %s, key %s, embeddings %s, database %s"),
        *GetActiveProviderId(),
        *FromUtf8(WanaMemory::ActiveModel(Backend->Settings)),
        Backend->bKeyPresent ? TEXT("set") : TEXT("missing"),
        *FromUtf8(Backend->Settings.Embeddings),
        *Backend->DatabasePath);
}

void UWanaMemorySubsystem::BroadcastWrite(const FString& CharacterId, const FString& PlayerId, const FString& WriteKind)
{
    OnMemoryWrittenNative.Broadcast(CharacterId, PlayerId, WriteKind);
    OnMemoryWritten.Broadcast(CharacterId, PlayerId, WriteKind);
}

void UWanaMemoryLibrary::TalkToCharacter(UObject* WorldContextObject, FString CharacterId, FString PlayerId, FString PlayerMessage, FWanaMemoryReplyDelegate OnReply)
{
    UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(WorldContextObject);
    UWanaMemorySubsystem* Subsystem = GameInstance ? GameInstance->GetSubsystem<UWanaMemorySubsystem>() : nullptr;
    if (!Subsystem)
    {
        OnReply.ExecuteIfBound(false, TEXT("WanaWorks memory is not available. Enable the WanaWorksMemory module first."));
        return;
    }
    Subsystem->TalkToCharacter(CharacterId, PlayerId, PlayerMessage, OnReply);
}
