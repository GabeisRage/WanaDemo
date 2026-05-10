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

Approximate completion: **58–60%**

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

---

## Current Immediate Focus

Work phases from here should focus on:

1. Runtime WanaAnimation adapter / consumption layer
2. Visible Character / AI Improvement
3. WIT Environment Scan V1
4. Level Design Workspace V1
5. UE5 Assistant Generation V1

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

**Status:** Current recommended next animation phase.

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

---

## Phase 6 — Visible Character / AI Improvement

**Status:** Next major demo impact phase after runtime adapter layer.

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

**Status:** Upcoming.

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

**Status:** Upcoming.

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
