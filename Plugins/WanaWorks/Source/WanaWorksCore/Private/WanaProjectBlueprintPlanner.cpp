#include "WanaProjectBlueprintPlanner.h"

#include "Misc/DateTime.h"

namespace
{
constexpr int32 MaxVisionCharacters = 4000;
constexpr int32 MaxPillars = 6;
constexpr int32 MaxSystems = 18;
constexpr int32 MaxGaps = 12;

bool ContainsAny(const FString& Source, std::initializer_list<const TCHAR*> Keywords)
{
    for (const TCHAR* Keyword : Keywords)
    {
        if (Source.Contains(Keyword, ESearchCase::IgnoreCase))
        {
            return true;
        }
    }
    return false;
}

FString NormalizeBrief(const FString& Brief)
{
    FString Result = Brief.Left(MaxVisionCharacters).TrimStartAndEnd();
    Result.ReplaceInline(TEXT("\r"), TEXT(" "));
    Result.ReplaceInline(TEXT("\n"), TEXT(" "));
    while (Result.Contains(TEXT("  ")))
    {
        Result.ReplaceInline(TEXT("  "), TEXT(" "));
    }
    return Result;
}

void AddUniqueString(TArray<FString>& Values, const FString& Value)
{
    if (!Value.IsEmpty())
    {
        Values.AddUnique(Value);
    }
}

void AddPillar(
    FWanaProjectBlueprintPlan& Plan,
    const FString& Name,
    const FString& Definition,
    const FString& PlayerValue,
    std::initializer_list<const TCHAR*> Systems,
    const FString& WorkspaceRoute)
{
    if (Plan.GameplayPillars.Num() >= MaxPillars)
    {
        return;
    }

    FWanaProjectBlueprintPillar Pillar;
    Pillar.Name = Name;
    Pillar.Definition = Definition;
    Pillar.PlayerValue = PlayerValue;
    Pillar.WorkspaceRoute = WorkspaceRoute;
    for (const TCHAR* System : Systems)
    {
        Pillar.RequiredSystems.Add(System);
    }
    Plan.GameplayPillars.Add(MoveTemp(Pillar));
}

void AddSystem(
    FWanaProjectBlueprintPlan& Plan,
    const FString& Name,
    std::initializer_list<const TCHAR*> Dependencies,
    const FString& Readiness,
    const FString& Evidence,
    const FString& WorkspaceRoute)
{
    if (Plan.RequiredSystems.Num() >= MaxSystems)
    {
        return;
    }

    if (Plan.RequiredSystems.ContainsByPredicate([&Name](const FWanaProjectBlueprintSystemRequirement& Existing)
        {
            return Existing.Name.Equals(Name, ESearchCase::IgnoreCase);
        }))
    {
        return;
    }

    FWanaProjectBlueprintSystemRequirement System;
    System.Name = Name;
    System.Readiness = Readiness;
    System.Evidence = Evidence;
    System.WorkspaceRoute = WorkspaceRoute;
    for (const TCHAR* Dependency : Dependencies)
    {
        System.Dependencies.Add(Dependency);
    }
    Plan.RequiredSystems.Add(MoveTemp(System));
}

void AddGap(
    FWanaProjectBlueprintPlan& Plan,
    const FString& Capability,
    const FString& Status,
    const FString& Evidence,
    const FString& WorkspaceRoute)
{
    if (Plan.Gaps.Num() >= MaxGaps)
    {
        return;
    }

    FWanaProjectBlueprintGap Gap;
    Gap.Capability = Capability;
    Gap.Status = Status;
    Gap.Evidence = Evidence;
    Gap.WorkspaceRoute = WorkspaceRoute;
    Plan.Gaps.Add(MoveTemp(Gap));
}

void AddRoute(
    FWanaProjectBlueprintPlan& Plan,
    const FString& Title,
    const FString& Reason,
    const FString& Workspace,
    const FString& WorkflowStep,
    const FString& Priority,
    const FString& Readiness,
    const FString& Risk)
{
    if (Plan.RecommendedRoutes.ContainsByPredicate([&Title](const FWanaProjectBlueprintRoute& Existing)
        {
            return Existing.Title.Equals(Title, ESearchCase::IgnoreCase);
        }))
    {
        return;
    }

    FWanaProjectBlueprintRoute Route;
    Route.Title = Title;
    Route.Reason = Reason;
    Route.Workspace = Workspace;
    Route.WorkflowStep = WorkflowStep;
    Route.Priority = Priority;
    Route.Readiness = Readiness;
    Route.Risk = Risk;
    Plan.RecommendedRoutes.Add(MoveTemp(Route));
}

void AddMilestone(
    FWanaProjectBlueprintPlan& Plan,
    const FString& StableId,
    const FString& Title,
    const FString& Objective,
    std::initializer_list<const TCHAR*> Systems,
    std::initializer_list<const TCHAR*> Dependencies,
    const FString& Evidence,
    const FString& WorkspaceRoute,
    const FString& Complexity = TEXT("Medium"),
    const FString& Readiness = TEXT("Needs Validation"))
{
    FWanaProjectBlueprintMilestone Milestone;
    Milestone.StableId = StableId;
    Milestone.Title = Title;
    Milestone.Objective = Objective;
    Milestone.CompletionEvidence = Evidence;
    Milestone.WorkspaceRoute = WorkspaceRoute;
    Milestone.Complexity = Complexity;
    Milestone.Readiness = Readiness;
    for (const TCHAR* System : Systems)
    {
        Milestone.RequiredSystems.Add(System);
    }
    for (const TCHAR* Dependency : Dependencies)
    {
        Milestone.DependsOn.Add(Dependency);
    }
    Plan.Milestones.Add(MoveTemp(Milestone));
}

int32 GetScopeCapacityScore(const FWanaProjectVisionBrief& Brief)
{
    int32 Score = 2;
    if (Brief.TeamSize.Contains(TEXT("2-5"), ESearchCase::IgnoreCase)
        || Brief.TeamSize.Contains(TEXT("Small"), ESearchCase::IgnoreCase))
    {
        Score += 2;
    }
    else if (Brief.TeamSize.Contains(TEXT("6-15"), ESearchCase::IgnoreCase))
    {
        Score += 4;
    }
    else if (Brief.TeamSize.Contains(TEXT("16+"), ESearchCase::IgnoreCase)
        || Brief.TeamSize.Contains(TEXT("Studio"), ESearchCase::IgnoreCase))
    {
        Score += 5;
    }

    if (Brief.ExperienceLevel.Contains(TEXT("Professional"), ESearchCase::IgnoreCase))
    {
        Score += 3;
    }
    else if (Brief.ExperienceLevel.Contains(TEXT("Experienced"), ESearchCase::IgnoreCase)
        || Brief.ExperienceLevel.Contains(TEXT("Advanced"), ESearchCase::IgnoreCase))
    {
        Score += 2;
    }
    else if (Brief.ExperienceLevel.Contains(TEXT("Beginner"), ESearchCase::IgnoreCase))
    {
        Score -= 1;
    }

    if (Brief.Timeline.Contains(TEXT("12"), ESearchCase::IgnoreCase)
        || Brief.Timeline.Contains(TEXT("18"), ESearchCase::IgnoreCase)
        || Brief.Timeline.Contains(TEXT("Long"), ESearchCase::IgnoreCase))
    {
        Score += 2;
    }
    else if (Brief.Timeline.Contains(TEXT("1 Month"), ESearchCase::IgnoreCase)
        || Brief.Timeline.Contains(TEXT("3 Months"), ESearchCase::IgnoreCase))
    {
        Score -= 1;
    }

    if (Brief.Budget.Contains(TEXT("Studio"), ESearchCase::IgnoreCase))
    {
        Score += 3;
    }
    else if (Brief.Budget.Contains(TEXT("Funded"), ESearchCase::IgnoreCase))
    {
        Score += 2;
    }
    else if (Brief.Budget.Contains(TEXT("Minimal"), ESearchCase::IgnoreCase))
    {
        Score -= 1;
    }
    return FMath::Max(1, Score);
}

FString BuildVisionSummary(const FString& NormalizedBrief, const FString& GameType, const FString& Perspective)
{
    if (NormalizedBrief.IsEmpty())
    {
        return TEXT("No vision brief yet. Add the player fantasy, signature interaction, and intended experience before analysis.");
    }

    FString Summary = NormalizedBrief.Left(260);
    if (NormalizedBrief.Len() > 260)
    {
        Summary += TEXT("...");
    }
    return FString::Printf(TEXT("%s | Interpreted as %s with a %s presentation."), *Summary, *GameType, *Perspective);
}
}

FWanaProjectBlueprintPlan FWanaRuleBasedProjectBlueprintPlanner::BuildPlan(
    const FWanaProjectVisionBrief& Brief,
    const FWanaProjectBlueprintReadinessContext& Readiness) const
{
    FWanaProjectBlueprintPlan Plan;
    Plan.PlanId = FString::Printf(TEXT("WW-Blueprint-%u"), GetTypeHash(Brief.VisionBrief + Brief.ProjectIdentifier));
    Plan.EngineAdapterId = Brief.EngineAdapterId.IsEmpty() ? TEXT("Unreal") : Brief.EngineAdapterId;
    Plan.GeneratedLabel = FDateTime::Now().ToString(TEXT("%Y-%m-%d %H:%M"));

    const FString Vision = NormalizeBrief(Brief.VisionBrief);
    Plan.bHasUsableVision = Vision.Len() >= 20;

    const bool bFirstPerson = ContainsAny(Vision, { TEXT("first person"), TEXT("first-person"), TEXT("fps") });
    const bool bTopDown = ContainsAny(Vision, { TEXT("top down"), TEXT("top-down"), TEXT("isometric") });
    const bool bMultiplayer = ContainsAny(Vision, { TEXT("multiplayer"), TEXT("co-op"), TEXT("coop"), TEXT("online") });
    const bool bOpenWorld = ContainsAny(Vision, { TEXT("open world"), TEXT("open-world"), TEXT("sandbox world") });
    const bool bCombat = ContainsAny(Vision, { TEXT("combat"), TEXT("fight"), TEXT("shooter"), TEXT("melee"), TEXT("enemy"), TEXT("soulslike"), TEXT("uncharted") });
    const bool bTraversal = ContainsAny(Vision, { TEXT("traversal"), TEXT("climb"), TEXT("parkour"), TEXT("platform"), TEXT("uncharted"), TEXT("metroidvania") });
    const bool bStealth = ContainsAny(Vision, { TEXT("stealth"), TEXT("sneak"), TEXT("infiltrat") });
    const bool bPuzzle = ContainsAny(Vision, { TEXT("puzzle"), TEXT("investigation"), TEXT("mystery"), TEXT("resident evil") });
    const bool bDialogue = ContainsAny(Vision, { TEXT("dialogue"), TEXT("conversation"), TEXT("narrative choice"), TEXT("relationship") });
    const bool bInventory = ContainsAny(Vision, { TEXT("inventory"), TEXT("craft"), TEXT("loot"), TEXT("survival"), TEXT("resident evil") });
    const bool bCompanion = ContainsAny(Vision, { TEXT("companion"), TEXT("ally"), TEXT("squad"), TEXT("uncharted") });
    const bool bHorror = ContainsAny(Vision, { TEXT("horror"), TEXT("terror"), TEXT("resident evil") });
    const bool bRpg = ContainsAny(Vision, { TEXT("rpg"), TEXT("role-playing"), TEXT("quest"), TEXT("level up") });
    const bool bCinematic = ContainsAny(Vision, { TEXT("cinematic"), TEXT("set piece"), TEXT("set-piece"), TEXT("uncharted") });
    const bool bProcedural = ContainsAny(Vision, { TEXT("procedural"), TEXT("roguelike"), TEXT("rogue-like") });
    const bool bAbilityGated = ContainsAny(Vision, { TEXT("metroidvania"), TEXT("ability gated"), TEXT("ability-gated") });
    const bool bDeliberateCombat = ContainsAny(Vision, { TEXT("soulslike"), TEXT("souls-like"), TEXT("deliberate melee") });
    const bool bAAAQuality = ContainsAny(Vision, { TEXT("aaa"), TEXT("triple-a"), TEXT("triple a") });
    const bool bVehicles = ContainsAny(Vision, { TEXT("vehicle"), TEXT("driving"), TEXT("racing"), TEXT("spaceship") });
    const bool bQuests = ContainsAny(Vision, { TEXT("quest"), TEXT("mission system"), TEXT("questline") });

    Plan.CameraPerspective = bFirstPerson ? TEXT("first-person") : bTopDown ? TEXT("top-down") : TEXT("third-person");
    Plan.InterpretedGameType = bHorror
        ? TEXT("survival horror")
        : bRpg ? TEXT("action role-playing")
        : bPuzzle && !bCombat ? TEXT("puzzle adventure")
        : bCombat ? TEXT("action adventure")
        : TEXT("character-driven adventure");
    Plan.PlayerFantasy = bHorror
        ? TEXT("survive pressure through observation, resource choices, and deliberate movement")
        : bDeliberateCombat ? TEXT("master readable encounters through committed movement and combat decisions")
        : bTraversal ? TEXT("move through authored spaces with confidence and physical expression")
        : bDialogue ? TEXT("shape relationships and outcomes through meaningful character choices")
        : TEXT("embody a capable character whose actions create clear, readable consequences");
    Plan.VisionSummary = BuildVisionSummary(Vision, Plan.InterpretedGameType, Plan.CameraPerspective);

    int32 Complexity = 2;
    Complexity += bCombat ? 2 : 0;
    Complexity += bTraversal ? 2 : 0;
    Complexity += bStealth ? 1 : 0;
    Complexity += bPuzzle ? 1 : 0;
    Complexity += bDialogue ? 1 : 0;
    Complexity += bInventory ? 1 : 0;
    Complexity += bCompanion ? 2 : 0;
    Complexity += bCinematic ? 2 : 0;
    Complexity += bMultiplayer ? 5 : 0;
    Complexity += bOpenWorld ? 4 : 0;
    Complexity += bProcedural ? 3 : 0;
    Complexity += bAAAQuality ? 4 : 0;
    Complexity += bVehicles ? 3 : 0;
    Complexity += bQuests ? 1 : 0;
    Complexity += Brief.IntendedDeliverable.Contains(TEXT("Full Game"), ESearchCase::IgnoreCase) ? 2 : 0;
    Complexity += Brief.TargetPlatform.Contains(TEXT("Multi"), ESearchCase::IgnoreCase) ? 2 : 0;

    const int32 Capacity = GetScopeCapacityScore(Brief);
    Plan.ProductionComplexity = Complexity >= 11 ? TEXT("High") : Complexity >= 7 ? TEXT("Medium-High") : TEXT("Focused");
    if (!Plan.bHasUsableVision)
    {
        Plan.ScopeReality = TEXT("Needs Definition");
        Plan.ScopeReason = TEXT("The constraints are available, but the vision is too short to produce a dependable scope recommendation.");
    }
    else if (Complexity > Capacity + 5)
    {
        Plan.ScopeReality = TEXT("Unrealistic Under Current Constraints");
        Plan.ScopeReason = TEXT("The requested system count and production dependencies exceed the stated team, experience, timeline, or budget. Prove one signature loop before expanding breadth.");
    }
    else if (Complexity > Capacity + 2)
    {
        Plan.ScopeReality = TEXT("Vertical Slice Recommended");
        Plan.ScopeReason = TEXT("The concept is feasible as a tightly authored proof, but the full feature set should remain outside the first production commitment.");
    }
    else
    {
        Plan.ScopeReality = TEXT("Focused Scope Feasible");
        Plan.ScopeReason = TEXT("The stated constraints can support a bounded prototype or vertical slice when milestones stay dependency-driven.");
    }

    if (bAAAQuality && !Plan.ScopeReality.Equals(TEXT("Focused Scope Feasible"), ESearchCase::IgnoreCase))
    {
        Plan.ScopeReason = TEXT("AAA production scale is not realistic under the stated constraints, but the quality aspiration can remain high. Concentrate polish on one short, authored proof instead of attempting AAA content breadth, staffing, and production volume.");
    }

    Plan.RecommendedDeliverable = Plan.ScopeReality.Equals(TEXT("Focused Scope Feasible"))
        ? Brief.IntendedDeliverable
        : TEXT("Vertical Slice");

    FString SliceCore = bCombat
        ? TEXT("one readable encounter")
        : bPuzzle ? TEXT("one complete puzzle chain")
        : bDialogue ? TEXT("one consequential character interaction")
        : TEXT("one complete signature interaction loop");
    if (bTraversal)
    {
        SliceCore += TEXT(" connected by a short traversal route");
    }
    if (bPuzzle)
    {
        SliceCore += TEXT(", one environmental puzzle");
    }
    if (bCompanion)
    {
        SliceCore += TEXT(" with one bounded companion behavior");
    }
    if (bCinematic)
    {
        SliceCore += TEXT(", one scripted cinematic transition");
    }
    Plan.RecommendedVerticalSlice = FString::Printf(
        TEXT("Build a %s %s experience containing %s, a clear start/end state, and a restartable validation path. Exclude open-world breadth, content scale, and secondary progression until this proof is stable."),
        *Brief.DesiredPlaytime,
        *Plan.InterpretedGameType,
        *SliceCore);
    Plan.VerticalSliceProof = TEXT("A new tester can enter, understand the player goal, complete the signature loop, encounter a recoverable failure, and reach a clear end without developer explanation.");

    AddPillar(Plan, TEXT("Readable Character Control"), TEXT("Movement, camera, and response make intent immediately legible."), TEXT("The player feels in control before content complexity is introduced."), { TEXT("Playable Character"), TEXT("Camera"), TEXT("Input") }, TEXT("Character Building"));
    if (bTraversal)
    {
        AddPillar(Plan, TEXT("Purposeful Traversal"), TEXT("Routes ask the player to read space and commit to understandable movement choices."), TEXT("Navigation itself carries pacing and mastery."), { TEXT("Traversal"), TEXT("Environment Semantics"), TEXT("Animation") }, TEXT("Level Design"));
    }
    if (bCombat)
    {
        AddPillar(Plan, bDeliberateCombat ? TEXT("Committed Encounter Decisions") : TEXT("Readable Encounter Pressure"), TEXT("Threats telegraph intent and reward positioning, timing, and recovery."), TEXT("Success feels earned rather than arbitrary."), { TEXT("Combat"), TEXT("Enemy Intelligence"), TEXT("Physical Reactions") }, TEXT("Character Intelligence"));
    }
    if (bStealth)
    {
        AddPillar(Plan, TEXT("Legible Stealth Information"), TEXT("Sight, sound, suspicion, and cover states are observable and consistent."), TEXT("Players can plan instead of guessing."), { TEXT("Perception"), TEXT("Enemy Intelligence"), TEXT("Environment Semantics") }, TEXT("Character Intelligence"));
    }
    if (bPuzzle)
    {
        AddPillar(Plan, TEXT("Cause-and-Effect Discovery"), TEXT("Clues and interactions form a bounded chain with visible consequences."), TEXT("Insight produces satisfying forward motion."), { TEXT("Interaction"), TEXT("Puzzle State"), TEXT("Feedback") }, TEXT("Logic & Blueprints"));
    }
    if (bDialogue || bCompanion)
    {
        AddPillar(Plan, TEXT("Believable Character Presence"), TEXT("Relationships and reactions remain consistent with role and context."), TEXT("Characters feel intentional rather than decorative."), { TEXT("Relationship Context"), TEXT("Dialogue"), TEXT("Character Intelligence") }, TEXT("Character Intelligence"));
    }
    if (Plan.GameplayPillars.Num() < 3)
    {
        AddPillar(Plan, TEXT("Clear Goal and Feedback"), TEXT("Every meaningful action communicates its purpose, result, and next state."), TEXT("The experience remains learnable without external explanation."), { TEXT("Interaction"), TEXT("Feedback"), TEXT("Flow State") }, TEXT("Logic & Blueprints"));
        AddPillar(Plan, TEXT("Authored World Meaning"), TEXT("The environment supports the core loop through deliberate routes, obstacles, and landmarks."), TEXT("Space reinforces play instead of serving as decoration."), { TEXT("Environment Semantics"), TEXT("Level Flow") }, TEXT("Level Design"));
    }

    for (FWanaProjectBlueprintPillar& Pillar : Plan.GameplayPillars)
    {
        if (Pillar.WorkspaceRoute.Equals(TEXT("Character Building"), ESearchCase::IgnoreCase))
        {
            Pillar.Readiness = Readiness.CharacterBuildingStatus;
        }
        else if (Pillar.WorkspaceRoute.Equals(TEXT("Character Intelligence"), ESearchCase::IgnoreCase))
        {
            Pillar.Readiness = Readiness.CharacterIntelligenceStatus;
        }
        else if (Pillar.WorkspaceRoute.Equals(TEXT("Level Design"), ESearchCase::IgnoreCase))
        {
            Pillar.Readiness = Readiness.EnvironmentStatus;
        }
        else
        {
            Pillar.Readiness = TEXT("Future Workspace / Needs Validation");
        }
    }

    AddSystem(Plan, TEXT("Project Health Foundation"), {}, Readiness.ProjectHealthStatus, Readiness.bProjectHealthScanAvailable ? TEXT("Current Project Health evidence is available.") : TEXT("Project Health has not been evaluated in this planning pass."), TEXT("Project Health"));
    AddSystem(Plan, TEXT("Playable Character"), { TEXT("Project Health Foundation") }, Readiness.CharacterBuildingStatus, Readiness.CharacterEvidence.IsEmpty() ? TEXT("No Character Building subject evidence is selected.") : Readiness.CharacterEvidence, TEXT("Character Building"));
    AddSystem(Plan, TEXT("Camera and Input"), { TEXT("Playable Character") }, Readiness.CharacterBuildingStatus, TEXT("Validated through playable control, camera, and movement readiness."), TEXT("Character Building"));
    AddSystem(Plan, TEXT("Animation and Physical Feedback"), { TEXT("Playable Character") }, Readiness.AnimationAdapterStatus, Readiness.AdapterEvidence.IsEmpty() ? TEXT("WanaAnimation adapter evidence is unknown.") : Readiness.AdapterEvidence, TEXT("Character Building"));
    AddSystem(Plan, TEXT("Interaction and Flow State"), { TEXT("Playable Character") }, TEXT("Unknown"), TEXT("Project-wide interaction and flow systems are not scanned by Project Blueprint V1."), TEXT("Logic & Blueprints"));
    AddSystem(Plan, TEXT("Checkpoint and Save"), { TEXT("Interaction and Flow State") }, TEXT("Unknown"), TEXT("The vertical slice needs a restartable validation path; save/checkpoint readiness is not scanned in V1."), TEXT("Logic & Blueprints"));
    AddSystem(Plan, TEXT("UI and Player Feedback"), { TEXT("Interaction and Flow State") }, TEXT("Unknown"), TEXT("Player-facing goal, success, failure, and recovery feedback requires manual validation."), TEXT("UI / UX"));
    AddSystem(Plan, TEXT("Environment Semantics"), { TEXT("Project Health Foundation") }, Readiness.EnvironmentStatus, Readiness.EnvironmentEvidence.IsEmpty() ? TEXT("No current WIT environment report was detected.") : Readiness.EnvironmentEvidence, TEXT("Level Design"));
    AddSystem(Plan, TEXT("Validation and Build"), { TEXT("Interaction and Flow State") }, TEXT("Partially Ready"), TEXT("WanaWorks Analyze, Test, and Build workflows are available; feature-specific proof remains project work."), TEXT("Project Blueprint"));
    if (bTraversal || bAbilityGated)
    {
        AddSystem(Plan, TEXT("Traversal"), { TEXT("Playable Character"), TEXT("Environment Semantics"), TEXT("Animation and Physical Feedback") }, TEXT("Unknown"), TEXT("The brief requests traversal; implementation readiness requires subject and environment validation."), TEXT("Character Building"));
    }
    if (bCombat || bStealth || bCompanion)
    {
        AddSystem(Plan, TEXT("Enemy Intelligence"), { TEXT("Project Health Foundation"), TEXT("Environment Semantics") }, Readiness.CharacterIntelligenceStatus, Readiness.IntelligenceEvidence.IsEmpty() ? TEXT("No Character Intelligence subject evidence is selected.") : Readiness.IntelligenceEvidence, TEXT("Character Intelligence"));
    }
    if (bCombat)
    {
        AddSystem(Plan, TEXT("Combat"), { TEXT("Playable Character"), TEXT("Animation and Physical Feedback"), TEXT("Enemy Intelligence") }, TEXT("Unknown"), TEXT("Combat intent was detected, but full combat implementation is outside current WanaWorks readiness scanning."), TEXT("Logic & Blueprints"));
        AddSystem(Plan, TEXT("Health and Damage"), { TEXT("Combat"), TEXT("UI and Player Feedback") }, TEXT("Unknown"), TEXT("Encounter success, failure, and recovery rules require manual validation."), TEXT("Logic & Blueprints"));
    }
    if (bStealth)
    {
        AddSystem(Plan, TEXT("Perception and Suspicion"), { TEXT("Enemy Intelligence"), TEXT("Environment Semantics") }, Readiness.CharacterIntelligenceStatus, TEXT("Stealth needs consistent perception, suspicion, and fallback behavior."), TEXT("Character Intelligence"));
    }
    if (bPuzzle)
    {
        AddSystem(Plan, TEXT("Puzzle State"), { TEXT("Interaction and Flow State") }, TEXT("Unknown"), TEXT("Puzzle rules and persistence are not scanned by Project Blueprint V1."), TEXT("Logic & Blueprints"));
    }
    if (bDialogue)
    {
        AddSystem(Plan, TEXT("Dialogue and Narrative State"), { TEXT("Interaction and Flow State") }, TEXT("Unknown"), TEXT("Narrative state is inferred from the brief and has not been inspected."), TEXT("Logic & Blueprints"));
    }
    if (bInventory)
    {
        AddSystem(Plan, TEXT("Inventory and Resources"), { TEXT("Interaction and Flow State") }, TEXT("Unknown"), TEXT("Resource pressure is a design requirement; implementation evidence is not currently scanned."), TEXT("Logic & Blueprints"));
    }
    if (bCompanion)
    {
        AddSystem(Plan, TEXT("Companion Behavior"), { TEXT("Enemy Intelligence"), TEXT("Relationship Context") }, Readiness.CharacterIntelligenceStatus, Readiness.IntelligenceEvidence, TEXT("Character Intelligence"));
        AddSystem(Plan, TEXT("Relationship Context"), { TEXT("Enemy Intelligence") }, Readiness.CharacterIntelligenceStatus, TEXT("WAY context should be verified on the selected companion subject."), TEXT("Character Intelligence"));
    }
    if (bCinematic)
    {
        AddSystem(Plan, TEXT("Cinematic Sequencing"), { TEXT("Interaction and Flow State"), TEXT("Animation and Physical Feedback") }, TEXT("Unknown"), TEXT("Cinematic transitions are inferred from the brief and require authored sequencing validation."), TEXT("Logic & Blueprints"));
    }
    if (bQuests || bRpg)
    {
        AddSystem(Plan, TEXT("Quest and Progression State"), { TEXT("Interaction and Flow State"), TEXT("Checkpoint and Save") }, TEXT("Unknown"), TEXT("Quest or progression intent is present, but implementation evidence is not scanned in V1."), TEXT("Logic & Blueprints"));
    }
    if (bVehicles)
    {
        AddSystem(Plan, TEXT("Vehicle Control"), { TEXT("Camera and Input"), TEXT("Environment Semantics") }, TEXT("Unknown"), TEXT("Vehicle intent is present and needs a separate movement, camera, collision, and animation proof."), TEXT("Logic & Blueprints"));
    }
    if (bMultiplayer)
    {
        AddSystem(Plan, TEXT("Network Play"), { TEXT("Project Health Foundation"), TEXT("Interaction and Flow State") }, TEXT("Unknown"), TEXT("Replication, hosting, authority, latency, and multiplayer testing are not inspected by Project Blueprint V1."), TEXT("Logic & Blueprints"));
    }
    if (bProcedural)
    {
        AddSystem(Plan, TEXT("Procedural Content Rules"), { TEXT("Environment Semantics"), TEXT("Interaction and Flow State") }, TEXT("Unknown"), TEXT("Procedural generation is inferred from the brief and remains a future/manual validation area."), TEXT("Logic & Blueprints"));
    }
    if (bAbilityGated)
    {
        AddSystem(Plan, TEXT("Ability-Gated Progression"), { TEXT("Traversal"), TEXT("Checkpoint and Save") }, TEXT("Unknown"), TEXT("Ability-gated routes require persistent progression state and authored return paths."), TEXT("Logic & Blueprints"));
    }
    AddGap(Plan, TEXT("Project Health"), Readiness.ProjectHealthStatus, Readiness.bProjectHealthScanAvailable ? TEXT("Diagnosis evidence can inform sequencing.") : TEXT("Run Analyze to establish project-level evidence."), TEXT("Project Health"));
    AddGap(Plan, TEXT("Playable Character"), Readiness.CharacterBuildingStatus, Readiness.CharacterEvidence.IsEmpty() ? TEXT("No selected Character Building evidence.") : Readiness.CharacterEvidence, TEXT("Character Building"));
    if (bCombat || bStealth || bCompanion)
    {
        AddGap(Plan, TEXT("Character Intelligence"), Readiness.CharacterIntelligenceStatus, Readiness.IntelligenceEvidence.IsEmpty() ? TEXT("No selected AI/NPC evidence.") : Readiness.IntelligenceEvidence, TEXT("Character Intelligence"));
    }
    AddGap(Plan, TEXT("Environment Semantics"), Readiness.EnvironmentStatus, Readiness.EnvironmentEvidence.IsEmpty() ? TEXT("No current WIT report evidence.") : Readiness.EnvironmentEvidence, TEXT("Level Design"));
    AddGap(Plan, TEXT("Animation Integration"), Readiness.AnimationAdapterStatus, Readiness.AdapterEvidence.IsEmpty() ? TEXT("Persistent adapter evidence is unknown.") : Readiness.AdapterEvidence, TEXT("Character Building"));
    for (const FWanaProjectBlueprintSystemRequirement& System : Plan.RequiredSystems)
    {
        if (Plan.Gaps.Num() >= MaxGaps)
        {
            break;
        }
        if (System.Readiness.Equals(TEXT("Unknown"), ESearchCase::IgnoreCase))
        {
            AddGap(Plan, System.Name, TEXT("Unknown"), System.Evidence, System.WorkspaceRoute);
        }
    }

    AddMilestone(Plan, TEXT("M01"), TEXT("Foundation and Scope Lock"), TEXT("Resolve critical project health issues and lock the vertical-slice boundaries."), { TEXT("Project Health Foundation") }, {}, TEXT("No critical blocker remains unexplained; the slice has explicit inclusions and exclusions."), TEXT("Project Health"), TEXT("Low-Medium"), Readiness.ProjectHealthStatus);
    AddMilestone(Plan, TEXT("M02"), TEXT("Playable Character Proof"), TEXT("Prove movement, camera, input, animation, and physical feedback on the selected character stack."), { TEXT("Playable Character"), TEXT("Camera and Input"), TEXT("Animation and Physical Feedback") }, { TEXT("M01") }, TEXT("A packaged or PIE test supports the full basic control loop without manual repair."), TEXT("Character Building"), TEXT("Medium"), Readiness.CharacterBuildingStatus);
    if (bCombat || bStealth || bCompanion)
    {
        AddMilestone(Plan, TEXT("M03"), TEXT("Character Interaction Proof"), TEXT("Prove one bounded AI, threat, stealth, or companion interaction required by the brief."), { TEXT("Enemy Intelligence") }, { TEXT("M02") }, TEXT("The behavior is visible, testable, and reports safe fallback states."), TEXT("Character Intelligence"), TEXT("Medium-High"), Readiness.CharacterIntelligenceStatus);
    }
    else
    {
        AddMilestone(Plan, TEXT("M03"), TEXT("Signature Mechanic Proof"), TEXT("Implement and validate the smallest complete version of the experience's defining interaction."), { TEXT("Interaction and Flow State") }, { TEXT("M02") }, TEXT("The mechanic has an entry, success, failure or recovery, and clear feedback."), TEXT("Logic & Blueprints"), TEXT("Medium-High"), TEXT("Future Workspace / Needs Validation"));
    }
    AddMilestone(Plan, TEXT("M04"), TEXT("Authored Environment Proof"), TEXT("Build the smallest environment that gives the signature loop meaningful spatial context."), { TEXT("Environment Semantics") }, { TEXT("M02") }, TEXT("Routes, obstacles, landmarks, and encounter space support the intended player decisions."), TEXT("Level Design"), TEXT("Medium"), Readiness.EnvironmentStatus);
    AddMilestone(Plan, TEXT("M05"), TEXT("Vertical Slice Integration"), TEXT("Connect the proven systems into one restartable beginning-to-end experience."), { TEXT("Interaction and Flow State"), TEXT("Validation and Build") }, { TEXT("M03"), TEXT("M04") }, Plan.VerticalSliceProof, TEXT("Project Blueprint"), TEXT("High"), TEXT("Blocked by M03 and M04"));
    AddMilestone(Plan, TEXT("M06"), TEXT("Prove and Produce"), TEXT("Test the slice against its stated proof, document limitations, and create a clean build candidate."), { TEXT("Validation and Build") }, { TEXT("M05") }, TEXT("The slice passes its proof without hidden setup and known limitations are recorded."), TEXT("Project Blueprint"), TEXT("Medium"), TEXT("Blocked by M05"));

    if (!Readiness.bProjectHealthScanAvailable || !Readiness.ProjectHealthStatus.Equals(TEXT("Healthy"), ESearchCase::IgnoreCase))
    {
        AddRoute(Plan, TEXT("Establish Project Health Evidence"), TEXT("Planning confidence depends on unresolved project-level risks and dependencies."), TEXT("Project Health"), TEXT("Analyze"), TEXT("High"), TEXT("Ready"), TEXT("Low"));
    }
    if (!Readiness.bCharacterSubjectSelected)
    {
        AddRoute(Plan, TEXT("Select the Playable Character Foundation"), TEXT("The first production dependency is a validated playable body, controller, camera, and animation stack."), TEXT("Character Building"), TEXT("Analyze"), TEXT("High"), TEXT("Needs Subject"), TEXT("Low"));
    }
    if ((bCombat || bStealth || bCompanion) && !Readiness.bIntelligenceSubjectSelected)
    {
        AddRoute(Plan, TEXT("Select the AI or NPC Foundation"), TEXT("The brief depends on character behavior, but no Character Intelligence subject evidence is selected."), TEXT("Character Intelligence"), TEXT("Analyze"), TEXT("High"), TEXT("Needs Subject"), TEXT("Low"));
    }
    if (!Readiness.bEnvironmentReportAvailable)
    {
        AddRoute(Plan, TEXT("Establish Environment Readiness"), TEXT("The vertical slice needs movement and environment evidence before spatial milestones can be trusted."), TEXT("Level Design"), TEXT("Analyze"), TEXT("Medium"), TEXT("Ready"), TEXT("Low"));
    }
    AddRoute(Plan, TEXT("Lock the Vertical Slice Contract"), Plan.ScopeReason, TEXT("Project Blueprint"), TEXT("Enhance"), TEXT("High"), Plan.bHasUsableVision ? TEXT("Ready") : TEXT("Needs Brief"), TEXT("Low"));

    if (ContainsAny(Vision, { TEXT("uncharted") }))
    {
        AddUniqueString(Plan.Assumptions, TEXT("The named inspiration was translated into generic traits: cinematic third-person presentation, authored traversal, readable set pieces, and companion-aware pacing. No protected characters, story, levels, or assets are reproduced."));
    }
    if (ContainsAny(Vision, { TEXT("soulslike"), TEXT("souls-like") }))
    {
        AddUniqueString(Plan.Assumptions, TEXT("The named inspiration was translated into generic traits: committed actions, readable threats, recovery windows, encounter mastery, and checkpoint-driven iteration."));
    }
    if (ContainsAny(Vision, { TEXT("metroidvania") }))
    {
        AddUniqueString(Plan.Assumptions, TEXT("The genre shorthand was translated into generic traits: interconnected routes, ability-gated access, backtracking value, and landmark-led navigation."));
    }
    if (ContainsAny(Vision, { TEXT("resident evil") }))
    {
        AddUniqueString(Plan.Assumptions, TEXT("The named inspiration was translated into generic traits: resource pressure, authored tension, puzzle-linked progression, and deliberate spatial planning."));
    }
    if (bMultiplayer)
    {
        AddUniqueString(Plan.Assumptions, TEXT("Multiplayer substantially expands architecture, testing, hosting, replication, and content constraints; the V1 slice should prove only the minimum networked loop."));
    }
    if (bOpenWorld)
    {
        AddUniqueString(Plan.Assumptions, TEXT("Open-world breadth is treated as a future production target, not part of the recommended first slice."));
    }
    AddUniqueString(Plan.Assumptions, TEXT("Unknown means the capability was not evidenced by the current WanaWorks readiness sources; it does not prove the system is absent."));
    AddUniqueString(Plan.Limitations, TEXT("Project Blueprint V1 uses transparent keyword and constraint rules, not an LLM or generative gameplay system."));
    AddUniqueString(Plan.Limitations, TEXT("Readiness is strongest for WanaWorks Project Health, Character Building, Character Intelligence, WIT, and adapter evidence; broader gameplay systems remain conservative Unknown states."));
    AddUniqueString(Plan.Limitations, TEXT("The plan is advisory and non-destructive. It does not create gameplay code, levels, graphs, or modify project settings."));
    if (Plan.RequiredSystems.Num() >= MaxSystems)
    {
        AddUniqueString(Plan.Limitations, FString::Printf(TEXT("The visible required-system list is capped at %d high-value entries in V1; a broad brief may require manual decomposition of additional secondary systems."), MaxSystems));
    }

    return Plan;
}
