#include "WAIPersonalityComponent.h"

void UWAIPersonalityComponent::RememberEvent(const FString& Summary, float EmotionalImpact)
{
    FWAIMemoryEvent Event;
    Event.Summary = Summary;
    Event.EmotionalImpact = EmotionalImpact;
    Memories.Add(Event);
}
