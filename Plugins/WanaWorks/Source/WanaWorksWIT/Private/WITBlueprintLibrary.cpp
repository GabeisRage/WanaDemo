#include "WITBlueprintLibrary.h"

FWanaPredictionProfile UWITBlueprintLibrary::BuildPredictionProfile(FName ClassificationTag, EWanaBehaviorDomain Domain, float Confidence)
{
    FWanaPredictionProfile Profile;
    Profile.ClassificationTag = ClassificationTag;
    Profile.Domain = Domain;
    Profile.Confidence = FMath::Clamp(Confidence, 0.0f, 1.0f);
    return Profile;
}
