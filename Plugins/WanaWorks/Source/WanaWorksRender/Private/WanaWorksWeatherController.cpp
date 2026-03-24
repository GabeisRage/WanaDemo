#include "WanaWorksWeatherController.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/SkyLight.h"
#include "Engine/World.h"
#include "EngineUtils.h"

namespace
{
template <typename TActorType>
TActorType* FindFirstActorOfType(UWorld* World)
{
    if (!World)
    {
        return nullptr;
    }

    for (TActorIterator<TActorType> ActorIt(World); ActorIt; ++ActorIt)
    {
        return *ActorIt;
    }

    return nullptr;
}

struct FWeatherPresetSettings
{
    const TCHAR* Name = TEXT("Clear");
    float DirectionalLightIntensity = 10.0f;
    FLinearColor DirectionalLightColor = FLinearColor::White;
    float SkyLightIntensity = 1.0f;
    float FogDensity = 0.01f;
};

FWeatherPresetSettings GetPresetSettings(EWanaWeatherPreset Preset)
{
    switch (Preset)
    {
    case EWanaWeatherPreset::Overcast:
        return {
            TEXT("Overcast"),
            3.0f,
            FLinearColor(0.78f, 0.82f, 0.90f),
            0.55f,
            0.03f
        };

    case EWanaWeatherPreset::Storm:
        return {
            TEXT("Storm"),
            0.85f,
            FLinearColor(0.60f, 0.68f, 0.85f),
            0.25f,
            0.08f
        };

    case EWanaWeatherPreset::Clear:
    default:
        return {
            TEXT("Clear"),
            10.0f,
            FLinearColor(1.00f, 0.96f, 0.90f),
            1.00f,
            0.01f
        };
    }
}
}

FWanaWeatherApplyResult FWanaWorksWeatherController::ApplyPreset(UWorld* World, EWanaWeatherPreset Preset)
{
    FWanaWeatherApplyResult Result;

    if (!World)
    {
        Result.ErrorMessage = TEXT("Could not apply weather because no world is available.");
        return Result;
    }

    const FWeatherPresetSettings Settings = GetPresetSettings(Preset);
    Result.PresetName = Settings.Name;

    if (ADirectionalLight* DirectionalLight = FindFirstActorOfType<ADirectionalLight>(World))
    {
        if (UDirectionalLightComponent* LightComponent = DirectionalLight->GetComponent())
        {
            DirectionalLight->Modify();
            LightComponent->Modify();
            LightComponent->SetIntensity(Settings.DirectionalLightIntensity);
            LightComponent->SetLightColor(Settings.DirectionalLightColor, true);
            Result.UpdatedSystems.Add(TEXT("DirectionalLight"));
        }
    }

    if (ASkyLight* SkyLight = FindFirstActorOfType<ASkyLight>(World))
    {
        if (USkyLightComponent* LightComponent = SkyLight->GetLightComponent())
        {
            SkyLight->Modify();
            LightComponent->Modify();
            LightComponent->SetIntensity(Settings.SkyLightIntensity);
            LightComponent->RecaptureSky();
            Result.UpdatedSystems.Add(TEXT("SkyLight"));
        }
    }

    if (AExponentialHeightFog* HeightFog = FindFirstActorOfType<AExponentialHeightFog>(World))
    {
        if (UExponentialHeightFogComponent* FogComponent = HeightFog->GetComponent())
        {
            HeightFog->Modify();
            FogComponent->Modify();
            FogComponent->SetFogDensity(Settings.FogDensity);
            Result.UpdatedSystems.Add(TEXT("ExponentialHeightFog"));
        }
    }

    if (Result.UpdatedSystems.Num() == 0)
    {
        Result.ErrorMessage = TEXT("Weather commands require a DirectionalLight, SkyLight, or ExponentialHeightFog actor in the current level.");
        return Result;
    }

    Result.bSucceeded = true;
    return Result;
}
