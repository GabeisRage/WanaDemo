#pragma once

#include "CoreMinimal.h"
#include "WanaProjectBlueprintTypes.h"

class WANAWORKSCORE_API IWanaProjectBlueprintPlanningProvider
{
public:
    virtual ~IWanaProjectBlueprintPlanningProvider() = default;

    virtual FWanaProjectBlueprintPlan BuildPlan(
        const FWanaProjectVisionBrief& Brief,
        const FWanaProjectBlueprintReadinessContext& Readiness) const = 0;
};

class WANAWORKSCORE_API FWanaRuleBasedProjectBlueprintPlanner final : public IWanaProjectBlueprintPlanningProvider
{
public:
    virtual FWanaProjectBlueprintPlan BuildPlan(
        const FWanaProjectVisionBrief& Brief,
        const FWanaProjectBlueprintReadinessContext& Readiness) const override;
};
