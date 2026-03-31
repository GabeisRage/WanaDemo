#pragma once

#include "WanaWorksTypes.h"

class AActor;

class WANAWORKSCORE_API FWanaWorksEnvironmentReadiness
{
public:
    static FWanaMovementReadiness EvaluateMovementReadiness(AActor* ObserverActor, AActor* TargetActor);
};
