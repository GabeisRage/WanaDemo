# Wana Works Plugin Audit

## Scope

This audit checks the current plugin scaffold for structural correctness and likely Unreal Engine 5 compile safety. It does not add gameplay, UI, or rendering features.

## Current Module Declarations

The plugin is currently split into six runtime modules:

- `WanaWorksCore`
- `WanaWorksWIT`
- `WanaWorksWAY`
- `WanaWorksWAI`
- `WanaWorksUI`
- `WanaWorksRender`

The `.uplugin` file declares those six modules consistently with the current source tree.

## What Is Structurally Valid

- Each declared module has a matching `Source/<ModuleName>` folder, a `<ModuleName>.Build.cs` file, and a module implementation `.cpp` using `IMPLEMENT_MODULE`.
- Public module classes and public Unreal-facing types use module export macros, which is the safer default for cross-module linkage in UE5.
- Current `Build.cs` files are minimal and match the actual code footprint more closely than the earlier scaffold.
- Existing work for WIT, WAY, WAI, UI, and Render remains intentionally minimal:
  - WIT exposes a small Blueprint library.
  - WAY exposes a lightweight player profile component.
  - WAI exposes a lightweight personality component.
  - UI currently exposes only a basic module class.
  - Render currently exposes only a basic module class.
- The Python hook file and development plan remain non-invasive scaffolding artifacts.

## Structural Issues Checked

### Module declarations
- The plugin manifest and the source layout are consistent.
- Every declared runtime module now has matching source/build/module files.

### Build.cs dependencies
- `WanaWorksCore`, `WanaWorksUI`, and `WanaWorksRender` only depend on `Core`, `CoreUObject`, and `Engine`.
- `WanaWorksWIT`, `WanaWorksWAY`, and `WanaWorksWAI` depend on those same engine modules plus `WanaWorksCore`.
- No current module declares an obviously unused dependency such as `AIModule`.

### Source layout
- Public and private source folders are present for all six modules.
- Public headers and private module `.cpp` files follow Unreal's normal module structure.

### Export macros
- Public module classes use `WANAWORKS*_API` macros.
- Public `UCLASS` and `USTRUCT` declarations that may cross module boundaries also use export macros.

### Include hygiene
- Public headers are lightweight and include only the Unreal headers they directly rely on.
- Module implementation `.cpp` files include their own module header first, which is the preferred Unreal include order.
- No obvious circular include pattern is present in the current scaffold.

## Remaining Likely UE5 Compile Risks

- The plugin has not yet been compiled inside a real UE5 project, so Unreal Header Tool and Unreal Build Tool have not validated generated code, module exports, or build settings.
- The runtime modules are structurally safe as scaffolding, but they have not yet been exercised against target-specific build settings, editor builds, or packaged builds.
- `PythonScriptPlugin` is declared in the plugin descriptor; compile safety is unaffected, but runtime availability still depends on the host project enabling and supporting that plugin.
- The docs describe planned surfaces such as WIT/WAY/WAI/WAMI, but only the current minimal scaffolding exists. That is intentional, not a structural defect.

## Recommended Next Step Before Feature Work

Run a real UE5 compile in a host project so Unreal Header Tool and Unreal Build Tool can validate the scaffold end-to-end before any new systems are added.
