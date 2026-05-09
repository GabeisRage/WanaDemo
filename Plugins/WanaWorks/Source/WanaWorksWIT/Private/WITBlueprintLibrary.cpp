#include "WITBlueprintLibrary.h"

#include "WanaWorksCoreModule.h"
#include "GameFramework/Actor.h"

namespace
{
    bool ContainsKeyword(const FString& Source, const TArray<FString>& Keywords)
    {
        for (const FString& Keyword : Keywords)
        {
            if (Source.Contains(Keyword, ESearchCase::IgnoreCase))
            {
                return true;
            }
        }
        return false;
    }
}

FWanaPredictionProfile UWITBlueprintLibrary::BuildPredictionProfile(FName ClassificationTag, EWanaBehaviorDomain Domain, float Confidence)
{
    FWanaPredictionProfile Profile;
    Profile.ClassificationTag = ClassificationTag;
    Profile.Domain = Domain;
    Profile.Confidence = FMath::Clamp(Confidence, 0.0f, 1.0f);
    return Profile;
}

FWITObjectProfile UWITBlueprintLibrary::ClassifyActor(AActor* TargetActor)
{
    FWITObjectProfile Profile;

    if (!TargetActor)
    {
        Profile.AIUsageHint = TEXT("Null actor — cannot classify.");
        return Profile;
    }

    const FString ClassName = TargetActor->GetClass()->GetName();
    Profile.ObjectClass = FName(*ClassName);

    const FString ActorName = TargetActor->GetName();
    const FString Combined = ClassName + TEXT("|") + ActorName;

    if (ContainsKeyword(Combined, { TEXT("Wood"), TEXT("Crate"), TEXT("Plank"), TEXT("Tree"), TEXT("Log") }))
    {
        Profile.MaterialType = FName(TEXT("Wood"));
        Profile.bIsBreakable = true;
        Profile.bIsFlammable = true;
        Profile.bIsMovable = true;
    }
    else if (ContainsKeyword(Combined, { TEXT("Metal"), TEXT("Steel"), TEXT("Iron"), TEXT("Pipe") }))
    {
        Profile.MaterialType = FName(TEXT("Metal"));
        Profile.bIsBreakable = false;
        Profile.bIsMovable = true;
    }
    else if (ContainsKeyword(Combined, { TEXT("Glass"), TEXT("Window"), TEXT("Mirror") }))
    {
        Profile.MaterialType = FName(TEXT("Glass"));
        Profile.bIsBreakable = true;
        Profile.bIsMovable = false;
    }
    else if (ContainsKeyword(Combined, { TEXT("Stone"), TEXT("Rock"), TEXT("Brick"), TEXT("Concrete") }))
    {
        Profile.MaterialType = FName(TEXT("Stone"));
        Profile.bIsBreakable = false;
        Profile.bIsMovable = false;
    }
    else if (ContainsKeyword(Combined, { TEXT("Cloth"), TEXT("Fabric"), TEXT("Curtain"), TEXT("Cushion") }))
    {
        Profile.MaterialType = FName(TEXT("Fabric"));
        Profile.bIsFlammable = true;
        Profile.bIsMovable = true;
    }
    else
    {
        Profile.MaterialType = FName(TEXT("Unknown"));
    }

    if (TargetActor->IsRootComponentMovable())
    {
        Profile.bIsMovable = true;
    }

    for (const FName& Tag : TargetActor->Tags)
    {
        Profile.SuggestedTags.AddUnique(Tag);
    }

    if (Profile.bIsBreakable && Profile.bIsFlammable)
    {
        Profile.AIUsageHint = TEXT("Highly destructible cover. AI can use as breachable obstacle or ignite for distraction.");
    }
    else if (Profile.bIsBreakable)
    {
        Profile.AIUsageHint = TEXT("Breakable object. AI may smash for shortcut or noise distraction.");
    }
    else if (Profile.bIsMovable)
    {
        Profile.AIUsageHint = TEXT("Movable prop. AI may push, pull, or use as mobile cover.");
    }
    else
    {
        Profile.AIUsageHint = TEXT("Static structural element. AI should treat as cover and pathing obstacle.");
    }

    Profile.SuggestedTags.AddUnique(Profile.MaterialType);

    FWanaLogger::Verbose(FString::Printf(TEXT("Classified %s as material=%s breakable=%d movable=%d"),
        *ActorName, *Profile.MaterialType.ToString(), Profile.bIsBreakable, Profile.bIsMovable));

    return Profile;
}

FString UWITBlueprintLibrary::GetObjectSummary(const FWITObjectProfile& Profile)
{
    FString Tags;
    for (const FName& Tag : Profile.SuggestedTags)
    {
        if (!Tags.IsEmpty())
        {
            Tags += TEXT(", ");
        }
        Tags += Tag.ToString();
    }

    return FString::Printf(
        TEXT("Object: %s\nMaterial: %s\nBreakable: %s | Movable: %s | Flammable: %s\nTags: %s\nAI Hint: %s"),
        *Profile.ObjectClass.ToString(),
        *Profile.MaterialType.ToString(),
        Profile.bIsBreakable ? TEXT("Yes") : TEXT("No"),
        Profile.bIsMovable ? TEXT("Yes") : TEXT("No"),
        Profile.bIsFlammable ? TEXT("Yes") : TEXT("No"),
        Tags.IsEmpty() ? TEXT("(none)") : *Tags,
        *Profile.AIUsageHint);
}
