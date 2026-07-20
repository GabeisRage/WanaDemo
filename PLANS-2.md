# WanaWorks Product Roadmap (PLANS-2.md v3)

> This file is the single source of truth for what to build and in what order. Agent behavior rules live in AGENTS.md — do not repeat them here.

---

## Product Vision

WanaWorks is a premium UE5 workspace platform and autonomous developer-assistance layer.

Target experience: investor-demo-quality, centered on Character Intelligence, Character Building, and Level Design. Beautiful shell. Working subject preview. Simple autonomous workflow. The user should feel like they opened professional creative software — not a plugin panel.

WanaWorks is not a generic prompt-output generator. It is a project-aware production intelligence system.

---

## WanaWorks P6™ Methodology

Every WanaWorks workspace should follow:

**Perceive → Preserve → Prepare → Perform → Prove → Produce**

UI button mapping:

| UI Button | P6 Meaning |
|---|---|
| Analyze | Perceive + readiness diagnosis |
| Enhance | Prepare + Perform |
| Test | Prove |
| Build | Produce |

The user-facing workflow stays simple, but the internal product methodology should remain proprietary and consistent.

---

## Current Product Status

### Demo/prototype foundation

Approximate completion: **92%**

### Full WanaWorks vision

Approximate completion: **58–60%** (rough estimate, predates the 2026-07-19 audit — Phases 5A and 8 moved from "in progress"/"upcoming" to verified-complete since this number was last set; treat the per-phase statuses below as ground truth over this percentage)

### Established foundation

The following are established and should not be regressed:

- Premium Studio Shell and Visual Identity
- Real Workspace Routing
- Live Preview / Studio Viewport
- Autonomous Subject Workflow V1
- Analyze / Enhance / Test / Build V1
- Character Intelligence behavior V1
- WAI/WAY/WIT influence V1
- WanaAnimation hook-state reporting
- WanaAnimation preview consumption
- Character Building profile/controller/shared-stack compatibility
- Anim BP readiness and generated adapter reporting
- Persistent WanaAnimation adapter report asset
- Runtime WanaAnimation Adapter Component V1 (Phase 5A — full field set surfaced in UI as of 2026-07-19)
- Level Design WIT Environment Scan V1 (Phase 8 — heuristic cover/obstacle/movement-space/boundary scan, persisted report asset, wired through Enhance/Test/Analyze/Build as of 2026-07-19)

> **Audit note (2026-07-19):** the two items above were previously tracked below as "Upcoming"/"in progress" phases, but a full source-level audit confirmed they are actually implemented end-to-end. See the updated Phase 5A and Phase 8 sections for what was verified and what still has gaps.

---

## Current Immediate Focus

Work phases from here should focus on:

1. Visible Character / AI Improvement (Phase 6) — the real remaining gap; the impact-to-physical-state chain computes correctly, but nothing yet visibly changes the character mesh in the viewport
2. WIT Depth (Phase 11) — WIT signals currently only produce textual guidance in Character Intelligence, not actual runtime AI behavior changes
3. UE5 Assistant Generation V1 (Phase 9)
4. WAI / WAMI Depth (Phase 10)

Runtime WanaAnimation Adapter (Phase 5A), WIT Environment Scan V1, and Level Design Workspace V1 (Phase 8) are now implemented end-to-end (verified 2026-07-19) — do not restart these from scratch.

Do not spend time on old utility/command features unless the user explicitly requests them.

Do not redo established UI shell work unless fixing bugs, polish regressions, or explicit user-requested UI refinements.

---

## Demo-Ready Definition

A demo-ready WanaWorks build must show:

1. Premium software shell visible
2. Subject picked from inside WanaWorks
3. Stack auto-detected, no manual wiring
4. Subject appears in live preview
5. Enhance applies automatically
6. Test shows visible behavior/animation/physical state result
7. Build creates clean output asset/report in WanaWorks content folder
8. Original source asset preserved
9. UI looks investor-presentable
10. WanaWorks follows the P6™ loop clearly enough to explain in a demo

---

## Phase 1 — Premium Studio Shell and Visual Identity

**Status:** Established foundation. Do not redo unless fixing regressions.

**Goal:** Make WanaWorks look and feel like premium standalone software inside Unreal Engine.

**Established behavior:**

- Left rail workspace nav
- Center hero stage / preview
- Right status cards
- Bottom action strip with Enhance / Test / Analyze / Build
- Premium dark WanaWorks visual direction
- Custom style system in `WanaWorksUIStyle`

**Exit criteria:** Established.

---

## Phase 2 — Real Workspace Routing

**Status:** Established foundation. Do not redo unless fixing regressions.

**Goal:** Each workspace rail button switches to a distinct, purpose-built body widget.

**Established active workspaces:**

| Workspace | Body focus |
|---|---|
| Character Intelligence | AI subject, WAI/WAMI, WAY-lite, WIT context, physical state, behavior |
| Character Building | Character subject, skeletal/rig/anim, identity, playable readiness, build readiness |
| Level Design | WIT / semantic world context, environment readiness, cover/obstacle/movement meaning |

**Exit criteria:** Established.

---

## Phase 3 — Live Preview / Studio Viewport

**Status:** Established foundation. Continue improving only when needed by later phases.

**Goal:** The center stage shows the selected subject and feels like a studio viewport.

**Established behavior:**

- Selected subjects can appear in the preview stage where supported
- View controls are present
- Preview is no longer a dead black placeholder in supported flows
- Stage supports WanaAnimation status/reporting surfaces

**Exit criteria:** Established.

---

## Phase 4 — Autonomous Subject Workflow

**Status:** Established foundation.

**Goal:** Remove manual selection and wiring friction from the primary user flow.

**Established behavior:**

- Subject picking inside WanaWorks
- Stack detection
- Working-copy / safe output concepts
- Workspace-aware Analyze / Enhance / Test / Build
- Quiet build behavior

**Exit criteria:** Established.

---

## Phase 5 — Automatic Anim BP Integration

**Status:** In progress / partially established.

**Goal:** Auto-detect and prepare WanaWorks animation integration without requiring the user to open Anim BP manually.

**Established behavior:**

- WanaAnimation hook state reporting
- WanaAnimation preview consumption readiness
- Anim BP readiness and shared-stack risk reporting
- Persistent WanaAnimation adapter report asset

**Next target:** Runtime WanaAnimation Adapter Component V1.

---

## Phase 5A — Runtime WanaAnimation Adapter Component V1

**Status:** Established foundation (verified complete 2026-07-19).

**Goal:** Create a runtime-safe WanaWorks-owned adapter component or helper layer that reads WanaAnimation hook state and exposes animation-friendly values for future Anim BP or linked-layer consumption, without editing the user’s original Anim BP.

**Target systems:**

- `WanaAutoAnimationIntegrationComponent`
- `WanaAnimationAdapterReportAsset`
- `WAYPlayerProfileComponent`
- Character Intelligence cards
- Character Building Rig & Animation cards

**Required behavior:**

- Adapter component/helper can read the persistent adapter report where practical
- Exposes posture/reaction/behavior/fallback/instability/recovery values
- Works for AI and playable character contexts
- Respects shared Anim BP safety
- Does not modify original Anim BP assets
- Reports readiness in Character Intelligence and Character Building

**Exit criteria:**

- Runtime-safe adapter values are available without editing Anim BP graphs
- UI reports adapter runtime readiness
- Missing systems degrade to Limited / Needs Enhance / Not Supported
- Original Anim BP remains untouched

**Verified 2026-07-19:** `UWanaAutoAnimationIntegrationComponent` performs real reflection-based `FProperty` writes onto a live `AnimInstance` when matching property names exist, never edits the Anim BP asset itself, and exposes the full posture/reaction/behavior/fallback/instability/recovery field set through `FWanaRuntimeAnimationAdapterState`. That full field set is now surfaced in the Character Intelligence / Character Building readiness detail string (the last 9 of 19 fields were wired in on 2026-07-19; the first 10 already existed). Readiness correctly degrades to Limited/Needs Enhance/Not Supported when no compatible Anim BP is present. This phase is done — do not restart it.

---

## Phase 6 — Visible Character / AI Improvement

**Status:** Not yet met — this is the real next focus (verified 2026-07-19).

**Ground truth as of 2026-07-19:** the Impact → Physical State leg is genuinely implemented (`UWanaPhysicalStateComponent` computes real Stable/Alert/Staggered/OffBalance/Bracing/Recovering state from impact hints, and it correctly reaches the runtime adapter). The Physical State → visible Animation/Behavior Response leg is **not implemented**: nothing in the plugin plays a Montage, drives an AnimNotify, blends a physics reaction, or otherwise changes what the skeletal mesh actually looks like. The existing "visible reaction simulation" feature (Character Intelligence "Test") only pushes text rows into the plugin's own analysis report — it does not move the character. The reflection-write auto-integration only takes effect if the target Anim Blueprint happens to expose exactly-matching property names, which the demo's ALS-based AnimBP almost certainly does not, so auto-wire likely applies 0 fields against the real demo subject today. Per AGENTS.md, directly editing the user's Anim BP graph is not allowed without explicit approval — so the practical path here is a plugin-owned procedural effect (e.g. a lean/stagger transform applied to the skeletal mesh component, driven by `StabilityScore`/`InstabilityAlpha`) layered on top of whatever the Anim BP already does, not an AnimGraph edit.

**Goal:** Deliver a dramatic enough visible behavior change that a non-technical viewer notices without reading logs.

**Required visible chain:**

Impact → Physical State → Staggered / OffBalance / Recovering → Animation/Behavior Response

**Target systems:**

- `WanaPhysicalStateComponent`
- `WanaAutoAnimationIntegrationComponent`
- `WAYPlayerProfileComponent`
- `WAIEmotionComponent`
- Character Intelligence preview/test path

**Exit criteria:**

- Medium impact produces visible disruption on the test subject
- Stronger impact produces stronger off-balance and recovery arc
- Enhanced subject behaves visibly differently from baseline
- Result is observable in the preview stage or UE viewport without reading logs

---

## Phase 7 — Character Building Workspace V1

**Status:** Established foundation with continuing enhancements.

**Established behavior:**

- Character profile controls
- Playable controller/readiness reporting
- Shared Character BP / mesh / skeleton / Anim BP reporting
- Rig and animation readiness
- Build readiness reporting

**Future improvements:**

- Deeper character-authoring tools
- Cover system generation
- playable mechanic scaffolds
- create-from-scratch character flow

---

## Phase 8 — Level Design Workspace V1

**Status:** V1 implemented and verified end-to-end (2026-07-19). Nearly all exit criteria below are already met by working code — see verification notes.

**Goal:** Make Level Design a real semantic world workspace powered by WIT.

**Must include:**

- environment context card
- WIT scan trigger
- obstacle / cover / movement-space result display
- scene utility context
- modular asset context
- atmosphere / vibe context

**Must not include:**

- AI pawn picker
- WAI identity cards
- Character Building tools

**Exit criteria:**

- WIT scan can be triggered from the Level Design workspace
- Results show semantic classification: cover / obstacle / movement space
- Scene and utility tools are in Level Design, not Character Intelligence
- Level Design does not fake modular generation before it exists

**Verified 2026-07-19:** the Level Design "Analyze/Enhance/Test/Build" tiles genuinely trigger `RunWITEnvironmentScan()`, which scans selection/world actors and classifies them into Cover / Obstacle / MovementSpace / Boundary / NavigationRelevant via name-keyword and geometry/collision heuristics (not ML/vision). Results render live in Level Design's "Semantic World Model" and "Environment Meaning" cards. Build persists a real, inspectable `WanaWITEnvironmentReportAsset` `.uasset` under `/Game/WanaWorks/WITReports`. "Full Level Generation" is explicitly hard-coded to "Not Supported" with an honest in-UI message, correctly satisfying the "does not fake modular generation" criterion. **Known gap:** this logic currently lives inline in `WanaWorksUIModule.cpp` rather than being exposed through the `WanaWorksWIT` module / `WITBlueprintLibrary` as originally scoped, so it isn't reusable from Blueprints or other C++ modules yet — a worthwhile follow-up, not a blocker.

---

## Phase 9 — UE5 Assistant Generation V1

**Status:** Upcoming.

**Goal:** WanaWorks can create at least one useful helper asset automatically.

**First generation targets:**

- AI-ready Controller scaffold
- helper Blueprint scaffold
- State Tree / Behavior Tree scaffold
- Blackboard scaffold
- WanaWorks adapter/helper assets
- organized WanaWorks output folders

**Exit criteria:**

- WanaWorks generates at least one asset that appears in the Content Browser
- Asset is inspectable and editable post-generation
- Original project assets are untouched
- Process feels assistant-like, not scripted/debug-like

---

## Phase 10 — WAI / WAMI Depth

**Status:** Upcoming.

**Goal:** AI identity, memory, emotion, and personality values affect observable behavior or animation — not just UI display.

**Target systems:**

- `WAIEmotionComponent`
- `WAIMemoryComponent`
- `WAIPersonalityComponent`
- `FWAITraitSet`
- `WAYPlayerProfileComponent`

**Exit criteria:**

- Observable behavioral difference between two AI subjects with different personality/emotion configurations
- Difference visible in preview stage or UE viewport
- WAI/WAMI affects behavior or animation state, not only text

---

## Phase 11 — WIT Depth

**Status:** Partially implemented (verified 2026-07-19) — display/guidance done, behavioral consumption not yet done.

**Goal:** WIT scan produces actionable scene meaning that influences AI behavior in Level Design and Character Intelligence.

**Target systems:**

- `WITBlueprintLibrary`
- `WanaWorksWITModule`
- Level Design workspace
- Character Intelligence behavior recommendation layer

**Exit criteria:**

- WIT scan produces cover / obstacle / movement-space classification
- At least one AI behavior reads and uses WIT output
- Level Design displays meaningful environment analysis

**Verified 2026-07-19:** the first and third criteria are met (see Phase 8). Character Intelligence does read the persisted WIT report back (`ResolveWITCharacterInfluenceContext`) and folds cover/obstacle/navigation signals into its suggested-improvement text — but this is textual guidance only. No evidence was found of it mutating an actual runtime AI/behavior component's decision logic. The "at least one AI behavior reads and uses WIT output" criterion is therefore not yet fully met — closing it means making WIT context change what a behavior actually does, not just what it says.

---

## Phase 11A — Workflow Action Planner V1

**Status:** Established foundation (verified 2026-07-20). This phase was already substantially built in commit `d2c11c6` before this entry existed — it was never given its own phase entry, which caused a full duplicate spec to be written for it later without realizing it already existed. Adding this entry so that doesn't happen again.

**Goal:** Convert existing per-workspace diagnosis/reporting into a prioritized, cross-workspace action-plan model — what to do next, why, how urgent, which workflow step handles it, whether WanaWorks can do it now or it needs a manual Unreal step.

**What's real:** `FWanaWorkflowActionPlanItem` (`WanaWorksUIModule.cpp`) — title, category, priority (Critical/High/Medium/Low/Informational), reason, source workspace, recommended workflow step (Analyze/Enhance/Test/Build/Manual Unreal Step/Future Automation), readiness state, risk level, executable-now flag, informational-only flag, related asset path, plus (added 2026-07-20, renamed 2026-07-20) `bRequiresManualEngineAction` (engine-neutral; Unreal-specific display wording like "Manual Unreal Step" is derived from this plus `EngineAdapterId` at display time, not embedded in the field), `bActionRequired` (true for an actionable/unresolved gap, false for an informational/readiness item — **current state only**), and `EngineAdapterId` (neutral engine identifier, always `"Unreal"` today) for the future multi-engine architecture. Generated per-workspace by `BuildCharacterIntelligenceWorkflowActionPlan` / `BuildCharacterBuildingWorkflowActionPlan` / `BuildLevelDesignWorkflowActionPlan`, sorted by priority, surfaced as the top 5 "Next Action" items in existing analysis cards across Analyze/Enhance/Test/Build. Cross-workspace guidance already works (e.g. Character Intelligence recommends "Run Level Design Build" when the WIT report is missing).

**Established, with one explicit limitation:** current action state (what's unresolved right now) is fully supported. **Historical resolution tracking is not implemented** — WanaWorks does not persist prior action-plan snapshots and cannot prove that a previously detected issue was actually corrected versus simply not re-detected this pass. `bActionRequired` reflects only the current readiness check, not a before/after comparison. Do not build features that assume WanaWorks remembers what it flagged last time until this is explicitly added.

**Exit criteria:** met. Do not restart this phase from scratch — check the actual code in `WanaWorksUIModule.cpp` before assuming a reporting/guidance feature doesn't exist.

---

## Phase 11B — Project Health Workspace V1

**Status:** Established foundation (2026-07-20).

**Goal:** A fourth live workspace giving developers an obvious way to inspect Unreal project health — engine version/compatibility, project modules, plugin dependencies, WanaWorks output readiness, build confidence. Read-only diagnosis and guidance only; no automatic repair.

**What's real:** `WanaWorksProjectHealthActions.h/.cpp` (new file) runs a real, read-only scan using `FEngineVersion::Current()`, `IProjectManager`/`IPluginManager` (project/plugin descriptors already in memory, no re-parsing), and a bounded text scan of each Runtime-type module's `Build.cs` for known editor-only dependency names. `FWanaWorksUIModule::RefreshProjectHealthWorkspaceState` reuses the existing shared `LastAnalysis*Summary` fields (same ones every other workspace already writes into) and the existing Workflow Action Planner (`BuildProjectHealthWorkflowActionPlan`, same model as the other three workspace planners) — no parallel scanner or action-planning system was created. Wired into Analyze/Enhance/Test/Build exactly like Level Design's branch. Rail entry added at position 4. No subject/preview concept — opted out of the sandbox preview system the same way Level Design already does.

**Exit criteria:** met. Do not restart this phase or re-audit whether a project-health/engine-version/module-scanning system exists — check `WanaWorksProjectHealthActions.h` and `RefreshProjectHealthWorkspaceState` in `WanaWorksUIModule.cpp` first.

**Known limitation carried forward intentionally:** no persistent Project Health report asset was added (V1 scope said not to widen scope for this); no top-bar status indicator was added (kept inside the workspace per the phase's own fallback instruction).

---

## Phase 12 — WanaCombat-lite

**Status:** Upcoming.

**Goal:** First tactical behavior layer — AI posture changes based on physical state, WIT cover, WAI emotion, and relationship state.

**Exit criteria:**

- AI visibly uses cover from WIT scan
- injury/stability state influences approach behavior
- hostility/fear/confidence meaningfully changes behavior labels and output
- no unsafe locomotion is forced

---

## Phase 13 — WanaAnimation Depth

**Status:** Upcoming after adapter/runtime foundation.

**Goal:** Embodiment and animation response become a major differentiator.

**Target systems:**

- `WanaAutoAnimationIntegrationComponent`
- `WanaAnimationAdapterReportAsset`
- runtime WanaAnimation adapter layer
- physical state component
- preview stage/test behavior

**Exit criteria:**

- Subject visibly reacts with stagger, lean, recovery, or emotional body language
- Supported cases require no manual Anim BP setup
- shared Anim BP risk is respected
- original Anim BP remains untouched unless user explicitly approves a supported integration path

---

## Phase 14 — Broader Platform Workspaces

**Status:** Future.

**Future workspaces:**

- Logic & Blueprints
- Physics
- Audio
- UI / UX
- Optimize
- Build & Deploy

**Exit criteria:**

- Each workspace has a purposeful body with real or honest placeholder content
- No empty panels
- No Character Intelligence copies
- Each lane reflects the WanaWorks P6™ loop

---

## Phase 15 — Synaptic Core / Advanced Assistant

**Status:** Long-term.

**Goal:** WanaWorks acts as an intelligent assistant — not just enhancement UI.

**Includes:**

- model-assisted authoring
- Blueprint/logic generation
- automated project cleanup
- analytics/tuning
- future cloud services if needed

**Exit criteria:**

- WanaWorks can author at least one piece of logic from a high-level user request
- outputs remain inspectable and editable
- non-destructive rule holds
- P6™ methodology remains visible in the user experience
