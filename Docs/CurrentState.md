# WanaWorks Current State

## Overview

This document records the current `WanaWorks` plugin layout in `WanaDemo` as of the Phase 0 baseline.

The plugin currently ships six modules:

- `WanaWorksCore`
- `WanaWorksWIT`
- `WanaWorksWAY`
- `WanaWorksWAI`
- `WanaWorksUI`
- `WanaWorksRender`

The current architecture is compile-safe and should be treated as the baseline for incremental work. In particular, `WanaWorksUI` is already the editor-facing module and currently owns editor tab registration.

## Plugin descriptor

Plugin file: `Plugins/WanaWorks/WanaWorks.uplugin`

- Plugin name: `Wana Works`
- Plugin category: `AI`
- Content enabled: `true`
- PythonScriptPlugin enabled: `true`
- Module loading phase for all plugin modules: `Default`

## Module layout

| Module | Type | Direct dependencies | Current responsibility |
| --- | --- | --- | --- |
| `WanaWorksCore` | Runtime | `Core`, `CoreUObject`, `Engine` | Base plugin module plus shared prediction types |
| `WanaWorksWIT` | Runtime | `Core`, `CoreUObject`, `Engine`, `WanaWorksCore` | Blueprint helper for constructing prediction profiles |
| `WanaWorksWAY` | Runtime | `Core`, `CoreUObject`, `Engine`, `WanaWorksCore` | Actor component for recording player preference signals |
| `WanaWorksWAI` | Runtime | `Core`, `CoreUObject`, `Engine`, `WanaWorksCore` | Actor component for storing simple memory events |
| `WanaWorksUI` | Editor | `Core`, `Slate`, `SlateCore` | Registers and spawns the `Wana Works` editor tab |
| `WanaWorksRender` | Runtime | `Core`, `CoreUObject`, `Engine` | Placeholder render module with startup and shutdown plumbing only |

## Module details

### WanaWorksCore

Source root: `Plugins/WanaWorks/Source/WanaWorksCore`

Current contents:

- `FWanaWorksCoreModule` implements empty `StartupModule()` and `ShutdownModule()` hooks
- `EWanaBehaviorDomain` defines the current behavior domains:
  `Environment`, `Character`, `Prop`, `Vehicle`, `Unknown`
- `FWanaPredictionProfile` stores:
  `ClassificationTag`, `Domain`, and `Confidence`

Current role:

- Serves as the shared base module for common types
- Provides the prediction profile struct consumed by higher-level systems

### WanaWorksWIT

Source root: `Plugins/WanaWorks/Source/WanaWorksWIT`

Current contents:

- `FWanaWorksWITModule` implements empty module startup and shutdown hooks
- `UWITBlueprintLibrary::BuildPredictionProfile(...)` constructs a `FWanaPredictionProfile`
- Confidence is clamped to the `[0.0, 1.0]` range before return

Current role:

- Exposes a minimal Blueprint-facing helper around `WanaWorksCore` prediction data

### WanaWorksWAY

Source root: `Plugins/WanaWorks/Source/WanaWorksWAY`

Current contents:

- `FWanaWorksWAYModule` implements empty module startup and shutdown hooks
- `FWAYPreferenceSignal` stores `SignalName` and `Weight`
- `UWAYPlayerProfileComponent` stores an array of preference signals on an actor
- `RecordPreferenceSignal(...)` appends a new signal entry
- `GetSignals()` exposes the recorded signal array

Current role:

- Stores simple player preference data at runtime

### WanaWorksWAI

Source root: `Plugins/WanaWorks/Source/WanaWorksWAI`

Current contents:

- `FWanaWorksWAIModule` implements empty module startup and shutdown hooks
- `FWAIMemoryEvent` stores `Summary` and `EmotionalImpact`
- `UWAIPersonalityComponent` stores an array of memory events on an actor
- `RememberEvent(...)` appends a new memory entry
- `GetMemories()` exposes the recorded memory array

Current role:

- Stores simple runtime memory and emotional context data

### WanaWorksUI

Source root: `Plugins/WanaWorks/Source/WanaWorksUI`

Current contents:

- Module type in the plugin descriptor is `Editor`
- Public module dependency surface is limited to `Core`
- Private editor/UI dependencies are `Slate` and `SlateCore`
- `FWanaWorksUIModule` registers a nomad tab spawner during startup
- Tab ID: `WanaWorksTab`
- Tab display name: `Wana Works`
- The tab is invoked on startup with `TryInvokeTab(...)`
- The tab body is a minimal Slate panel with a single text label:
  `Wana Works Initialized`
- Shutdown unregisters the tab spawner when Slate is initialized

Current role:

- Acts as the plugin's current editor-facing module
- Owns the existing Unreal Editor tab registration and minimal Slate UI

### WanaWorksRender

Source root: `Plugins/WanaWorks/Source/WanaWorksRender`

Current contents:

- `FWanaWorksRenderModule` implements empty module startup and shutdown hooks

Current role:

- Placeholder module for future rendering or look-preset work

## Supporting plugin content

Python hooks file: `Plugins/WanaWorks/Content/Python/wana_ai_hooks.py`

Current contents:

- `classify_object(object_name)` returns a placeholder classification payload
- `summarize_player_profile(signal_names)` returns a lightweight summary payload

Current role:

- Provides stub Python integration points for future editor or runtime integration

## Architecture notes for next phases

- `WanaWorksUI` already contains the working editor tab code; Phase 1 should stabilize this module rather than split it
- `WanaWorksUI` does not currently expose editor-only APIs in its public header beyond the module interface
- The current tab implementation compiles with a narrow dependency set: `Core` public, `Slate` and `SlateCore` private
- Runtime modules currently depend only on engine/runtime modules plus `WanaWorksCore` where needed
- No runtime module currently depends on editor-only code
- Most plugin modules are still scaffold-level and have intentionally minimal startup logic
