# WanaWorks Product Roadmap (PLANS-2.md v2)

> This file is the single source of truth for what to build and in what order. Agent behavior rules live in AGENTS.md — do not repeat them here.

---

## Current Immediate Focus

Work phases in this order:

1. Phase 1 — Premium Studio Shell and Visual Identity
2. Phase 2 — Real Workspace Routing
3. Phase 3 — Live Preview / Studio Viewport
4. Phase 4 — Autonomous Subject Workflow

Do not spend time on old utility/command features unless the user explicitly requests them.

---

## Product Vision

WanaWorks is a premium UE5 workspace platform and autonomous developer-assistance layer.

Target experience: investor-demo-quality, centered on Character Intelligence, Character Building, and Level Design. Beautiful shell. Working subject preview. Simple autonomous workflow. The user should feel like they opened professional creative software — not a plugin panel.

**Demo-ready definition** (all must be true):
1. Premium software shell visible
2. Subject picked from inside WanaWorks
3. Stack auto-detected, no manual wiring
4. Subject appears in live preview
5. Enhance applies automatically
6. Test shows visible behavior/animation/physical state result
7. Build creates clean output asset in WanaWorks content folder
8. Original source asset preserved
9. UI looks investor-presentable

---

## Phase 1 — Premium Studio Shell and Visual Identity

**Goal:** Make WanaWorks look and feel like premium standalone software inside Unreal Engine.

**Target files / systems:**
- `WanaWorksUI/Private/WanaWorksUIStyle.h/.cpp` — add named color constants, font helpers, card brush definitions
- `WanaWorksUI/Private/WanaWorksUITabBuilder.h/.cpp` — rebuild layout with left rail, center stage, right status column, bottom action strip
- `WanaWorksUI/Private/WanaWorksUIModule.cpp` — confirm tab registration and style init order

**Concrete behavior:**
- Left vertical rail: 3 workspace buttons (Character Intelligence, Character Building, Level Design) with active-state style; future workspaces shown as dimmed entries
- Center: `SOverlay` with dark-fill `SBorder` background labeled as the hero stage; placeholder subject preview label until Phase 3
- Right: 2–3 `SBorder` status cards (Subject, Readiness, Stack state)
- Bottom strip: exactly 4 `SButton` with `FButtonStyle` override — Enhance, Test, Analyze, Build
- No output log on the main face; no debug command buttons visible

**Exit criteria:**
- Screenshot does not read as a default Unreal tool or utility panel
- At least one custom `FSlateBrush` (defined in `WanaWorksUIStyle`) is used on a card surface
- All four main-face action buttons use `FButtonStyle` overrides, not naked defaults
- Typography uses named `FSlateFontInfo` constants from `WanaWorksUIStyle`

**Protected files for this phase:** `WAYPlayerProfileComponent.h/.cpp`, `WAYRelationshipTypes.h`, `WanaWorksTypes.h`, `WITBlueprintLibrary.h/.cpp`, `WanaWorks.uplugin`

**What good looks like:**
- Before: gray utility panel with rows of labeled buttons and an output log
- After: dark-themed studio layout with branded rail, dark center stage, right-column status cards, and a clean 4-button action strip

---

## Phase 2 — Real Workspace Routing

**Goal:** Each workspace rail button switches to a distinct, purpose-built body widget.

**Target files / systems:**
- `WanaWorksUI/Private/WanaWorksUITabBuilder.h/.cpp` — implement workspace body switcher
- New files (allowed): `WanaWorksCharacterIntelligenceBody.h/.cpp`, `WanaWorksCharacterBuildingBody.h/.cpp`, `WanaWorksLevelDesignBody.h/.cpp` (all inside `WanaWorksUI/Private/`)

**Concrete behavior:**

| Workspace | Body must show |
|---|---|
| Character Intelligence | AI subject picker, WAI/WAMI identity card, WAY-lite relationship chip, WIT context chip, physical state card, behavior result card |
| Character Building | Character subject picker, skeletal mesh card, rig/anim readiness card, identity card, build readiness status, Build action |
| Level Design | Environment context card, WIT scan trigger, obstacle/cover/movement-space result cards, scene utility strip |

Future workspaces (Logic & Blueprints, Physics, Audio, etc.) render as a polished "Coming Soon" body — not blank, not a copy of Character Intelligence.

**Exit criteria:**
- Clicking each of the 3 active workspaces produces a visually distinct body widget
- Character Building body contains no WAI/WAY debug buttons
- Level Design body contains no AI-pawn picker
- `SWidget` returned by each body builder compiles and renders without assertion

**Protected files for this phase:** Same as Phase 1, plus all UI files created in Phase 1 (do not regress style).

**What good looks like:**
- Before: all workspaces show the same widget body, only the rail label changes
- After: each workspace has its own card layout matching its domain; the Level Design body has no AI identity cards

---

## Phase 3 — Live Preview / Studio Viewport

**Goal:** The center stage shows the selected subject and feels like a studio viewport.

**Target files / systems:**
- `WanaWorksUI/Private/WanaWorksUITabBuilder.h/.cpp` or new `WanaWorksPreviewStage.h/.cpp` — preview stage widget
- `WanaWorksCore/Public/WanaSandboxManager.h` — use sandbox actor for preview subject
- UE5 Editor viewport embedding: `FPreviewScene` + `SEditorViewport` subclass inside `WanaWorksUI/Private/`

**Technical approach:**
- Embed a `SEditorViewport` subclass (private to WanaWorksUI) as the center stage widget
- Use `FPreviewScene` to host a cloned/working-copy actor without placing it in the active level
- Subject selection drives `UWanaSandboxManager` to spawn a preview instance into `FPreviewScene`
- View mode chips (Overview / Front / Back / Left / Right) call `SetViewLocation` + `SetViewRotation` on the embedded viewport client
- If `FPreviewScene` is not viable in one pass, render an `SImage` with the last captured thumbnail and label it clearly as "Static Preview"

**Concrete behavior:**
- When a subject is selected, preview stage shows that actor (or its working copy) in the embedded viewport
- View chips are connected; clicking Front changes camera orientation
- Stage is never a dead black rectangle in supported flows
- If preview is not yet live, show a styled empty state with subject name and a "Preview Pending" chip — never a silent black box

**Exit criteria:**
- Selected subject appears inside the center stage (live or static thumbnail)
- View chips respond with a visible change
- No silent black placeholder in supported Character Intelligence flows
- `SEditorViewport` subclass or thumbnail image compiles and renders without crash

**Protected files for this phase:** `WAYPlayerProfileComponent.h/.cpp`, `WAYRelationshipTypes.h`, `WanaWorksTypes.h`, `WITBlueprintLibrary.h/.cpp`

**What good looks like:**
- Before: center stage is a black or gray dead rectangle
- After: selected character pawn is visible and framed in the center stage; clicking "Front" reorients the camera

---

## Phase 4 — Autonomous Subject Workflow

**Goal:** Remove manual selection and wiring friction from the primary user flow.

**Target files / systems:**
- `WanaWorksCore/Public/UWanaProjectScanner.h/.cpp` — enumerate AI Pawns in the open level
- `WanaWorksCore/Public/UWanaSandboxManager.h/.cpp` — working copy creation
- `WanaWorksUI/Private/` workspace body files — populate subject picker from scanner results
- `WanaWorksWAI/Public/WAIEmotionComponent.h`, `WAIMemoryComponent.h`, `WAIPersonalityComponent.h` — read state for auto-detect display

**Concrete behavior:**
1. User opens WanaWorks — scanner enumerates AI Pawns in the active level automatically
2. Subject picker in Character Intelligence populates with found pawns; user taps one
3. WanaWorks reads: pawn type, Anim BP, skeletal mesh, AI Controller, WanaWorks component state, WAI component state
4. If required components are missing, `UWanaSandboxManager` creates a working copy and attaches them
5. Status cards update to reflect detected state (Ready / Applied / Limited / Not Supported)
6. Enhance, Test, Analyze, Build are now actionable

**Exit criteria:**
- User does not need to select an actor in the UE viewport as the primary path
- Scanner returns at least one pawn from an open level with a pawn actor
- Working copy is created without mutating the original Blueprint asset
- Status cards display detected component state, not placeholder text

**Protected files for this phase:** `WAYPlayerProfileComponent.h/.cpp`, `WAYRelationshipTypes.h`, `WanaWorksTypes.h`, `WITBlueprintLibrary.h/.cpp`, `WanaWorks.uplugin`

**What good looks like:**
- Before: user must click actor in UE viewport, manually create copy, manually attach components
- After: opening WanaWorks shows a populated subject list; tapping a pawn fills status cards automatically

---

## Phase 5 — Automatic Anim BP Integration

**Goal:** Auto-detect and auto-attach WanaWorks animation integration without the user opening Anim BP.

**Target files / systems:**
- `WanaWorksWAY/Public/WanaAutoAnimationIntegrationComponent.h/.cpp`
- `WanaWorksWAY/Public/WanaAnimationAdapterReportAsset.h`
- `WanaWorksUI/Private/` Character Intelligence body — integration state card

**Integration states:** Ready / Applied / Limited / Not Supported

**Exit criteria:**
- Supported cases complete without user opening Anim BP
- UI card shows integration state with one of the four labels
- Original Anim BP asset is not modified
- Unsupported cases display a clear, styled failure state (not a crash or log dump)

---

## Phase 6 — Visible Character / AI Improvement

**Goal:** Deliver a dramatic enough visible behavior change that a non-technical viewer notices without reading logs.

**Target files / systems:**
- `WanaWorksCore/Public/WanaPhysicalStateComponent.h/.cpp` — physical state pipeline
- `WanaWorksWAI/Public/WAIEmotionComponent.h/.cpp` — emotion influence on behavior
- `WanaWorksUI/Private/` Character Intelligence body — impact/behavior result card

**Required visible chain:** Impact → Physical State (Staggered / OffBalance / Recovering) → Animation/Behavior Response

**Exit criteria:**
- Medium impact produces a visible stagger/disruption on the test subject
- Stronger impact produces stronger off-balance and recovery arc
- Enhanced subject behaves visibly differently from baseline
- Result is observable in the preview stage or UE viewport without reading logs

---

## Phase 7 — Character Building Workspace V1

**Goal:** Make Character Building a real, distinct workspace for preparing character assets.

**Target files / systems:**
- `WanaWorksUI/Private/WanaWorksCharacterBuildingBody.h/.cpp`
- `WanaWorksWAY/Public/WanaIdentityComponent.h/.cpp`

**Must include:** character picker, character preview, skeletal/rig/anim awareness, identity context, build readiness card, build output action  
**Must not include:** WAI/WAY debug buttons, AI pawn stack cards, WIT scan tools

**Exit criteria:**
- Workspace body is visually and functionally distinct from Character Intelligence
- User understands this workspace is for building/preparing characters
- Preview and status cards reflect character-building context, not AI behavior context

---

## Phase 8 — Level Design Workspace V1

**Goal:** Make Level Design a real semantic world workspace powered by WIT.

**Target files / systems:**
- `WanaWorksUI/Private/WanaWorksLevelDesignBody.h/.cpp`
- `WanaWorksWIT/Public/WITBlueprintLibrary.h` — WIT scan exposure to UI

**Must include:** environment context card, WIT scan trigger, obstacle/cover/movement-space result display, scene utility strip  
**Must not include:** AI pawn picker, WAI identity cards, Character Building tools

**Exit criteria:**
- WIT scan can be triggered from the Level Design workspace
- Results show semantic classification (cover / obstacle / movement space) for scanned objects
- Scene and utility tools are in Level Design, not in Character Intelligence

---

## Phase 9 — UE5 Assistant Generation V1

**Goal:** WanaWorks can create at least one useful helper asset automatically.

**Target files / systems:**
- `WanaWorksCore/Public/UWanaProjectScanner.h` — asset existence checks
- `WanaWorksUI/Private/` — generation result card
- Generated output: `Content/WanaWorks/Generated/` folder in the project

**First generation targets:** AI-ready Controller scaffold, helper Blueprint scaffold, State Tree or Behavior Tree scaffold

**Exit criteria:**
- WanaWorks generates at least one asset that appears in the Content Browser
- Asset is inspectable and editable post-generation
- Original project assets are untouched
- Process feels assistant-like, not scripted/debug-like

---

## Phase 10 — WAI / WAMI Depth

**Goal:** AI identity, memory, emotion, and personality values affect observable behavior or animation — not just UI display.

**Target files / systems:** `WAIEmotionComponent.h/.cpp`, `WAIMemoryComponent.h/.cpp`, `WAIPersonalityComponent.h/.cpp` (with `FWAITraitSet`)

**Exit criteria:** Observable behavioral difference between two AI subjects with different personality/emotion configurations, visible in the preview stage or UE viewport.

---

## Phase 11 — WIT Depth

**Goal:** WIT scan produces actionable scene meaning that influences AI behavior in Level Design.

**Target files / systems:** `WITBlueprintLibrary.h/.cpp`, `WanaWorksWITModule`

**Exit criteria:** WIT scan produces cover / obstacle / movement-space classification; at least one AI behavior in Phase 12 reads and uses WIT output.

---

## Phase 12 — WanaCombat-lite

**Goal:** First tactical behavior layer — AI posture changes based on physical state, WIT cover, WAI emotion.

**Exit criteria:** AI visibly uses cover from WIT scan; injury/stability state from WanaPhysicalStateComponent influences approach behavior.

---

## Phase 13 — WanaAnimation Depth

**Goal:** Embodiment and animation response become a major differentiator.

**Target files / systems:** `WanaAutoAnimationIntegrationComponent.h/.cpp`, `WanaAnimationAdapterReportAsset.h`

**Exit criteria:** Subject visibly reacts (stagger, lean, recovery, emotional body language) without manual Anim BP setup for supported cases.

---

## Phase 14 — Broader Platform Workspaces

**Goal:** Expand beyond character intelligence to additional workspace lanes.

**Future workspaces:** Logic & Blueprints, Physics, Audio, UI/UX, Optimize, Build & Deploy  
**Exit criteria:** Each new workspace has a purposeful body with real (or honest placeholder) content — no empty panels or AI copies.

---

## Phase 15 — Synaptic Core / Advanced Assistant

**Goal:** WanaWorks acts as an intelligent assistant — not just enhancement UI.

**Includes:** Model-assisted authoring, Blueprint/logic generation, automated project cleanup, analytics, future cloud services  
**Exit criteria:** WanaWorks can author at least one piece of logic from a high-level user request; all outputs remain inspectable and editable; non-destructive rule holds.
