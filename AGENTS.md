# WanaWorks Agent Instructions (AGENTS.md v2)

> This file is the single source of truth for Codex agent behavior. PLANS-2.md is the single source of truth for what to build and in what order. Do not repeat rules from this file in PLANS-2.md.

---

## Session Startup Checklist

Before writing any code, execute these steps in order:

1. `git checkout master`
2. `git pull origin master`
3. Read this file (AGENTS.md) in full
4. Read PLANS-2.md in full
5. Identify the current phase
6. Check the Protected Files section below — do not touch those files
7. Confirm the project compiles before adding features

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
4. Report the error in your session result (see Reporting Contract)

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

---

## Current Product Status (as of May 2026)

**Demo/prototype foundation:** ~70% complete  
**Full WanaWorks vision:** ~38–40% complete  
**Canonical branch:** `master`  
**`main` branch:** stale — ignore

### What exists in WanaWorksCore
- `FWanaLogger` — structured log system
- `UWanaSettings` — plugin-level developer settings
- `UWanaProjectScanner` — project asset scanning
- `UWanaSandboxManager` — working copy / sandbox management
- `UWanaPhysicalStateComponent` — physical state tracking
- `UWanaWorksCommandDispatcher`, `UWanaWorksCommandRegistry` — command routing
- `UWanaWorksEnvironmentReadiness` — environment readiness evaluation
- `WanaWorksTypes.h` — all shared enums and structs (additive only)

### What exists in WanaWorksWAI
- `UWAIEmotionComponent` — emotion state
- `UWAIMemoryComponent` — event memory
- `UWAIPersonalityComponent` with `FWAITraitSet` — personality modeling

### What exists in WanaWorksWAY
- `WAYPlayerProfileComponent.h/.cpp` — advanced relationship/animation system
- `WAYRelationshipTypes.h` — relationship type definitions
- `WanaIdentityComponent`, `WanaAutoAnimationIntegrationComponent`, `WanaAnimationAdapterReportAsset`

### What exists in WanaWorksWIT
- `WITBlueprintLibrary.h/.cpp` — evolved movement readiness system

### What exists in WanaWorksUI (Editor-only — ahead, do not regress)
- `WanaWorksUIStyle.h/.cpp` — style system
- `WanaWorksUITabBuilder.h/.cpp` — tab/panel builder
- `WanaWorksUIFormattingUtils.h/.cpp` — formatting helpers
- `WanaWorksUIEditorActions.h/.cpp` — editor action bindings
- `WanaWorksUISummaryText.h/.cpp` — summary text generation

---

## Protected Files

Do not modify these files without explicit user instruction. Treat them as read-only.

| File | Reason |
|---|---|
| `WanaWorksUI/Private/WanaWorksUIStyle.h/.cpp` | Style system is ahead — do not regress |
| `WanaWorksUI/Private/WanaWorksUITabBuilder.h/.cpp` | Studio shell is ahead |
| `WanaWorksUI/Private/WanaWorksUIFormattingUtils.h/.cpp` | Established helpers |
| `WanaWorksUI/Private/WanaWorksUIEditorActions.h/.cpp` | Established actions |
| `WanaWorksUI/Private/WanaWorksUISummaryText.h/.cpp` | Established helpers |
| `WAYPlayerProfileComponent.h/.cpp` | Advanced relationship/animation system |
| `WAYRelationshipTypes.h` | Relationship type definitions — additive only |
| `WanaWorksTypes.h` | Never remove or change existing types — additive only |
| `WanaWorks.uplugin` | Do not change module types or branding without user approval |
| `WITBlueprintLibrary.h/.cpp` | Evolved movement readiness system |
| Any file containing `FWanaMovementReadiness` | Evolved type — additive only |
| Any file containing `FWAYRelationshipProfile` | Protected struct |
| Any file containing `FWAYAnimationHookState` | Protected struct |

If a task requires modifying a protected file, stop and ask the user.

---

## Module Dependency Rules

```
WanaWorksCore         → Engine, CoreUObject, Core, DeveloperSettings, AIModule only
WanaWorksWAI/WAY/WIT  → may depend on WanaWorksCore; never on WanaWorksUI
WanaWorksUI           → may depend on all runtime modules; Editor-only (LoadingPhase = EditorOnly)
WanaWorksRender       → Runtime; may depend on WanaWorksCore
```

- Never create circular dependencies
- Never include Editor-only headers (`#include "Editor/..."`, `#include "Widgets/..."`, UMG, etc.) inside Runtime modules
- Never add `UnrealEd` or `Slate`/`SlateCore` to a Runtime `.Build.cs`
- `WanaWorksUI` is the only module that may import Slate, SlateCore, EditorStyle, UnrealEd

---

## UI / Slate Rules

WanaWorks UI is implemented in Slate/C++, not UMG or HTML. Apply these rules to all UI work.

### Prohibited patterns
- Default `FEditorStyle` / plain `SButton` with no style override
- `SMultiLineEditableText` as primary user feedback
- Output log or `UE_LOG` as the primary feedback surface
- Raw table rows and `SDetailsView` as main workspace body
- Dense toolbar button piles as the primary face

### Required patterns
- Background fills: `FAppStyle::GetBrush("ToolPanel.DarkGroupBorder")` or a named brush from `WanaWorksUIStyle`
- Card surfaces: `SBorder` with a custom `FSlateBrush` defined in `WanaWorksUIStyle`
- Typography: `STextBlock` with `FSlateFontInfo` constants defined as `static FSlateFontInfo` helpers in `WanaWorksUIStyle.h`
- Buttons: `SButton` with full `FButtonStyle` override — never naked default buttons on primary surfaces
- Layered stage chrome: `SOverlay` with `SBorder` background and `SVerticalBox` content layers
- Status feedback: styled `STextBlock` inside a card, or a `SBorder` toast strip — not a log
- Navigation rail: `SVerticalBox` of `SButton` with active-state style swap
- Custom colors: `static FLinearColor` constants in `WanaWorksUIStyle.h`
- Custom fonts: `static FSlateFontInfo` helpers in `WanaWorksUIStyle.h`

### Layout structure target

```
[Left rail: workspace nav]  [Center: hero stage / preview]  [Right: status cards]
                            [Bottom: Enhance | Test | Analyze | Build strip]
```

### Premium checklist (UI work passes when ALL are true)
- [ ] No default UE toolbar styling on primary surfaces
- [ ] At least one `SBorder` with custom brush used as card surface
- [ ] Typography uses named `FSlateFontInfo` constants, not inline point sizes
- [ ] Status feedback uses card or toast, not log output
- [ ] Primary action strip shows at most: Enhance / Test / Analyze / Build

---

## Workspace Rules

Three workspaces must be meaningfully distinct:

| Workspace | Primary focus |
|---|---|
| Character Intelligence | AI Pawn, WAI/WAMI, WAY-lite, WIT context, physical state, behavior |
| Character Building | Character subject, skeletal/rig/anim, identity, build readiness, output |
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
- WanaWorks component attachment state
- Animation integration state
- Physical state status

Working copy / sandbox:
- Always create a working copy before mutating anything
- Originals are never touched
- Build output goes to WanaWorks content output folders only
- Never clutter the active viewport or steal Content Browser focus without user action

---

## Refactor and Design-System Freedom

Codex may refactor within `WanaWorksUI` when needed to support the current phase. Allowed without asking:
- Extracting new style/theme helpers into `WanaWorksUIStyle.h`
- Creating new workspace body builder classes in `WanaWorksUI/Private/`
- Replacing old visible UI structure in `WanaWorksUITabBuilder`
- Deleting obsolete debug-surface code from the main face

Not allowed without asking:
- Adding new plugin modules
- Breaking existing compile
- Modifying protected files (see above)
- Large rewrites to Core/WAI/WAY/WIT backend logic

---

## Reporting Contract (Mandatory)

Every session result must include all of the following. A session without all items is considered incomplete.

1. **Files changed** — list every file modified, created, or deleted
2. **Why they changed** — one sentence per file
3. **Compile result** — clean / failed + first error if failed
4. **Manual validation steps** — what the user must do to verify the change
5. **Suggested commit message**

For UI work, also include:
6. **What the user should visually check** — be specific (e.g., "left rail should show 3 workspace buttons with active state on Character Intelligence")
7. **Known limitations** — anything not yet implemented that the user should expect

If compile failed, only report items 1–3 and the fix applied. Do not report visual results from a broken build.

---

## Execution Rules

- Execute one phase at a time unless the user explicitly asks for a combined pass
- After each phase, verify the project compiles
- Stop after the phase completes — do not widen scope
- Preserve existing working systems unless the user explicitly says to replace or hide them
- Do not introduce new output log surfaces as primary feedback
- Do not reintroduce helper-button clutter removed in prior phases
