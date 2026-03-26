# Wana Works Plugin Development Plan

## 1. Plugin Structure

The Wana Works plugin is organized as a modular Unreal Engine 5 plugin with four runtime modules:

- **WanaWorksCore**: Shared types, common interfaces, and integration contracts.
- **WanaWorksWIT**: Object classification, semantic tagging, and predictive world behavior.
- **WanaWorksWAY**: Player preference tracking, adaptation, and interaction tuning.
- **WanaWorksWAI**: Character identity, memory, emotion, and personality simulation.

This separation keeps the system extensible while allowing individual modules to be enabled, profiled, and tested independently.

## 2. Core Functionalities

### WIT™ (What Is This)
- Uses ML-assisted or rules-assisted classification to assign semantic labels to actors and world objects.
- Converts classification output into gameplay-safe `FWanaPredictionProfile` values.
- Exposes Blueprint helpers so designers can annotate or override predicted object behavior.

### WAY™ (Who Are You)
- Records player preference signals as they make decisions in dialogue, combat, exploration, and crafting.
- Aggregates those signals into profiles that AI can reference when choosing tone, challenge, and support behavior.
- Supports replication-ready component design so multiplayer sessions can persist player adaptation states.

### WAI/WAMI™ (Who Am I)
- Stores memory events and emotional impact values for AI characters.
- Provides a starting point for personality simulation layers such as trust, fear, loyalty, and curiosity.
- Supports future behavior-tree, state-tree, and blackboard integration for long-term AI identity.

## 3. Unreal Engine 5 Integration

### Blueprint Nodes
- `UWITBlueprintLibrary::BuildPredictionProfile` gives designers a simple entry point for WIT classification payloads.
- `UWAYPlayerProfileComponent` can be attached to pawns, controllers, or profile actors.
- `UWAIPersonalityComponent` can be attached to AI actors to capture memories and emotional state.

### C++ API
- Shared structs and enums live in `WanaWorksCore` so all modules reference the same contracts.
- Each subsystem is isolated into its own runtime module for maintainability and future editor tooling.
- The scaffold is ready for expansion with interfaces, subsystem classes, data assets, and replication logic.

### Python Hooks via WanaAI
- `Content/Python/wana_ai_hooks.py` provides a placeholder integration point for Unreal's Python plugin.
- Hooks should remain advisory so game-critical logic still executes deterministically in C++ or Blueprints.
- Future work can forward payloads to external services, local inference, or offline content pipelines.

## 4. Installation and Deployment

1. Copy the `Plugins/WanaWorks` folder into your Unreal Engine 5 project.
2. Open the `.uproject` file and enable **Wana Works** from the Plugins browser.
3. Ensure the **Python Script Plugin** is enabled if you intend to use WanaAI Python hooks.
4. Regenerate project files, then build the project in your IDE or through Unreal Build Tool.
5. Add `UWAYPlayerProfileComponent` and `UWAIPersonalityComponent` to the relevant actors.
6. Use Blueprint or C++ entry points to feed classification, preference, and memory data.

## 5. Compatibility Targets

- **UE5 AI Systems**: Designed to integrate with Behavior Trees, EQS, StateTree, and AI Perception.
- **Physics Systems**: WIT can map object classes to predictable physical responses or affordances.
- **Networked Multiplayer**: WAY and WAI components should replicate only the minimum state needed for authoritative AI decisions.

## 6. Recommended Next Steps

1. Add replicated properties and server-authoritative RPCs to the WAY and WAI components.
2. Introduce data assets for object taxonomies, preference curves, and personality archetypes.
3. Add automated tests through Unreal's functional and automation testing frameworks.
4. Build editor tooling for debugging classifications, player profiles, and personality memory graphs.
5. Replace placeholder Python hooks with real WanaAI adapters and timeout-safe integration paths.
