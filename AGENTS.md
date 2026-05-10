# WanaWorks Agent Instructions (AGENTS.md v3)

> This file is the single source of truth for Codex agent behavior. PLANS-2.md is the single source of truth for what to build and in what order. Do not repeat rules from this file in PLANS-2.md.

---

## Session Startup Checklist

Before writing any code, execute these steps in order:

1. `git checkout master`
2. `git pull origin master`
3. Read this file (AGENTS.md) in full
4. Read PLANS-2.md in full
5. Identify the current phase
6. Check the High-Value Established Files section below
7. Confirm the project compiles before adding features, unless the active task is docs-only

Never start work on `main`. If you find yourself on `main`, stop and switch to `master`.

---

## Branch Rule

Always work on `master`. This is the canonical branch.  
`main` is stale — never commit to it, never merge from it.

---

## Compile Failure Rule

If a compile fails:

1. Stop immediately
2. Fix only the first reported error
3. Do not add new features until the project compiles cleanly
4. Report the error in your session result

---

## Product Truth

WanaWorks is a premium UE5 workspace platform and autonomous developer-assistance layer. It is not a plugin tab, utility panel, or debug dashboard.

It must feel like a standalone creative software product that lives inside Unreal Engine.

WanaWorks is:

- A premium software workspace inside UE5
- A non-destructive orchestration layer over UE5-native systems
- A platform with multiple workspaces
- An autonomous assistant that detects, attaches, wires, enhances, tests, and builds where safely possible
- A product that hides complexity behind simple workflows

The user selects a subject. WanaWorks detects the rest, creates a safe working version, enhances it, tests it, and builds the final output.

WanaWorks is not a prompt-output generator. WanaWorks is a project-aware production intelligence system. It must understand the project, preserve assets, validate results, and produce clean, inspectable outputs.

---

## WanaWorks P6™ Production Loop

WanaWorks follows the P6™ Production Loop:

**Perceive → Preserve → Prepare → Perform → Prove → Produce**

Definitions:

- **Perceive:** Detect what the selected subject is and how it is built.
- **Preserve:** Protect original assets and avoid destructive edits.
- **Prepare:** Create safe working context, hooks, profiles, readiness states, and output paths.
- **Perform:** Apply safe improvements and orchestration.
- **Prove:** Test, validate, explain readiness, limitations, and risk.
- **Produce:** Generate clean WanaWorks-owned outputs, reports, assets, or build artifacts.

User-facing buttons may remain simple:

- Analyze
- Enhance
- Test
- Build

Internally, all workspace behavior should respect the P6™ loop.

---

## Current Product Status (as of May 2026)

**Demo/prototype foundation:** ~92% complete  
**Full WanaWorks vision:** ~58–60% complete  
**Canonical branch:** `master`  
**`main` branch:** stale — ignore

### Established Product Foundation

The following systems are established and should not be regressed:

- Premium studio shell
- Real workspace routing
- Live preview / studio stage
- Analyze / Enhance / Test / Build V1
- Character Intelligence behavior upgrade
- WAI/WAY/WIT influence V1
- WanaAnimation hook-state reporting
- WanaAnimation preview consumption
- Character Building profile/controller/shared-stack compatibility
- Anim BP readiness and generated adapter reporting
- Persistent WanaAnimation adapter report asset

### What exists in WanaWorksCore

- `FWanaLogger` — structured log system
- `UWanaSettings` — plugin-level developer settings
- `UWanaProjectScanner` — project asset scanning
- `UWanaSandboxManager` — working copy / sandbox management
- `UWanaPhysicalStateComponent` — physical state tracking
- `UWanaWorksCommandDispatcher`, `UWanaWorksCommandRegistry` — command routing
- `UWanaWorksEnvironmentReadiness` — environment readiness evaluation
- `WanaWorksTypes.h` — all shared enums and structs

### What exists in WanaWorksWAI

- `UWAIEmotionComponent` — emotion state
- `UWAIMemoryComponent` — event memory
- `UWAIPersonalityComponent` with `FWAITraitSet` — personality modeling

### What exists in WanaWorksWAY

- `WAYPlayerProfileComponent.h/.cpp` — advanced relationship/animation system
- `WAYRelationshipTypes.h` — relationship type definitions
- `WanaIdentityComponent`
- `WanaAutoAnimationIntegrationComponent`
- `WanaAnimationAdapterReportAsset`

### What exists in WanaWorksWIT

- `WITBlueprintLibrary.h/.cpp` — evolved movement readiness / WIT support

### What exists in WanaWorksUI

- `WanaWorksUIStyle.h/.cpp` — style system
- `WanaWorksUITabBuilder.h/.cpp` — tab/panel builder
- `WanaWorksUIFormattingUtils.h/.cpp` — formatting helpers
- `WanaWorksUIEditorActions.h/.cpp` — editor action bindings
- `WanaWorksUISummaryText.h/.cpp` — summary text generation

---

## High-Value Established Files

These files contain established systems. They are not forbidden, but they must be handled carefully.

| File | Rule |
|---|---|
| `WanaWorksUI/Private/WanaWorksUIStyle.h/.cpp` | Additive, phase-scoped changes only. Do not regress the design system. |
| `WanaWorksUI/Private/WanaWorksUITabBuilder.h/.cpp` | Additive, phase-scoped changes only. Do not regress workspace routing or shell structure. |
| `WanaWorksUI/Private/WanaWorksUIFormattingUtils.h/.cpp` | Additive helper changes only. Do not remove established formatting behavior. |
| `WanaWorksUI/Private/WanaWorksUIEditorActions.h/.cpp` | Additive, phase-scoped action changes only. Do not break established workflows. |
| `WanaWorksUI/Private/WanaWorksUISummaryText.h/.cpp` | Additive summary/reporting changes only. Do not regress established status language. |
| `WAYPlayerProfileComponent.h/.cpp` | Additive, phase-scoped behavior/animation changes only. Do not regress relationship or hook behavior. |
| `WAYRelationshipTypes.h` | Additive only. Do not remove or rename existing relationship types. |
| `WanaWorksTypes.h` | Additive only. Never remove or destructively change existing types. |
| `WanaWorks.uplugin` | Do not change module types or branding without explicit user approval. |
| `WITBlueprintLibrary.h/.cpp` | Additive WIT changes only. Do not regress movement readiness. |
| Any file containing `FWanaMovementReadiness` | Additive only. |
| Any file containing `FWAYRelationshipProfile` | Additive only. |
| Any file containing `FWAYAnimationHookState` | Additive only. |

### Established File Policy

Do not rewrite, delete, rename, or regress these files.  
Do not remove existing behavior or change existing public types destructively.  
Additive, phase-scoped changes are allowed when the active phase explicitly requires them.  
If a phase requires touching one of these files, explain why in the final report.  
If a change would be destructive or broad, stop and ask the user.

---

## Module Dependency Rules

```text
WanaWorksCore         → Engine, CoreUObject, Core, DeveloperSettings, AIModule only
WanaWorksWAI/WAY/WIT  → may depend on WanaWorksCore; never on WanaWorksUI
WanaWorksUI           → may depend on all runtime modules; Editor-only
WanaWorksRender       → Runtime; may depend on WanaWorksCore
```

- Never create circular dependencies.
- Never include Editor-only headers inside Runtime modules.
- Never add `UnrealEd` or `Slate`/`SlateCore` to a Runtime `.Build.cs`.
- `WanaWorksUI` is the only module that may import Slate, SlateCore, EditorStyle, UnrealEd.

---

## UI / Slate Rules

WanaWorks UI is implemented in Slate/C++, not UMG or HTML.

### Prohibited patterns

- Default `FEditorStyle` / plain `SButton` with no style override on primary surfaces
- `SMultiLineEditableText` as primary user feedback
- Output log or `UE_LOG` as the primary feedback surface
- Raw table rows and `SDetailsView` as main workspace body
- Dense toolbar button piles as the primary face

### Required patterns

- Background fills: `FAppStyle::GetBrush("ToolPanel.DarkGroupBorder")` or a named brush from `WanaWorksUIStyle`
- Card surfaces: `SBorder` with a custom `FSlateBrush` defined in `WanaWorksUIStyle`
- Typography: `STextBlock` with `FSlateFontInfo` constants defined as helpers in `WanaWorksUIStyle.h`
- Buttons: `SButton` with full `FButtonStyle` override on primary surfaces
- Layered stage chrome: `SOverlay` with `SBorder` background and layered content
- Status feedback: styled `STextBlock` inside a card, or a `SBorder` toast strip
- Navigation rail: `SVerticalBox` of `SButton` with active-state style swap
- Custom colors: `static FLinearColor` constants in `WanaWorksUIStyle.h`
- Custom fonts: `static FSlateFontInfo` helpers in `WanaWorksUIStyle.h`

### Layout structure target

```text
[Left rail: workspace nav]  [Center: hero stage / preview]  [Right: status cards]
                            [Bottom: Enhance | Test | Analyze | Build strip]
```

### Premium checklist

UI work passes only when all are true:

- No default UE toolbar styling on primary surfaces
- At least one `SBorder` with a custom brush is used as a card surface
- Typography uses named `FSlateFontInfo` constants, not random inline point sizes
- Status feedback uses cards/toasts, not log output
- Primary action strip shows at most: Enhance / Test / Analyze / Build

---

## Workspace Rules

Three active workspaces must be meaningfully distinct:

| Workspace | Primary focus |
|---|---|
| Character Intelligence | AI Pawn, WAI/WAMI, WAY-lite, WIT context, physical state, behavior |
| Character Building | Character subject, skeletal/rig/anim, identity, playable readiness, output |
| Level Design | WIT scan, semantic environment, cover/obstacle/movement meaning |

Clicking a workspace rail item must change the actual body widget, not only the active label.

Future workspaces (Logic & Blueprints, Physics, Audio, UI/UX, Optimize, Build & Deploy) must render as polished, honest "coming soon" states — not blank panels or copies of Character Intelligence.

---

## Autonomy and Non-Destructive Rules

WanaWorks must auto-detect and auto-wire where safe. Manual setup is a fallback only.

Auto-detect on subject selection:

- Pawn type
- Linked Anim BP
- Skeletal mesh
- AI Controller
- Player Controller / playable control context where appropriate
- WanaWorks component attachment state
- Animation integration state
- Physical state status
- Shared player/AI asset stack risk

Working copy / sandbox:

- Always create a working copy before mutating anything
- Originals are never touched
- Build output goes to WanaWorks content output folders only
- Never clutter the active viewport or steal Content Browser focus without user action

---

## Refactor and Design-System Freedom

Codex may refactor within the WanaWorks plugin when needed to support the current phase.

Allowed without asking:

- Extracting new style/theme helpers into `WanaWorksUIStyle`
- Creating new workspace body builder classes in `WanaWorksUI/Private/`
- Replacing obsolete visible UI structure in `WanaWorksUITabBuilder`
- Deleting obsolete debug-surface code from the main face
- Adding runtime-safe components or data assets when the active phase requires them
- Adding WanaWorks-owned generated report/config assets when non-destructive

Not allowed without asking:

- Adding new plugin modules
- Breaking existing compile
- Destructive rewrites to established systems
- Changing module type or branding in `WanaWorks.uplugin`
- Directly modifying user Anim BP graphs, Behavior Trees, Blackboards, or State Trees unless the active phase explicitly allows it
- Replacing assigned Anim Classes automatically without explicit user approval

---

## Reporting Contract

Every session result must include:

1. **Files changed** — every file modified, created, or deleted
2. **Why they changed** — one sentence per file
3. **Compile result** — clean / failed + first error if failed
4. **Manual validation steps**
5. **Suggested commit message**

For UI work, also include:

6. **What the user should visually check**
7. **Known limitations**

For crash fixes, also include:

8. **Crash-safety checks added**

If compile failed, only report items 1–3 and the fix applied. Do not report visual results from a broken build.

---

## Execution Rules

- Execute one phase at a time unless the user explicitly asks for a combined pass
- After each code phase, verify the project compiles
- Stop after the phase completes — do not widen scope
- Preserve existing working systems unless the user explicitly says to replace or hide them
- Do not introduce new output log surfaces as primary feedback
- Do not reintroduce helper-button clutter removed in prior phases
