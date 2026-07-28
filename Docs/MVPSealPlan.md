# WanaWorks MVP Seal Plan

> Purpose: lock the MVP scope so the product can be sealed and demoed, instead of growing sideways. PLANS-2.md stays the source of truth for phase detail; this doc says what is **in** the MVP, what is **cut until after seal**, and what "sealed" means. Created 2026-07-28.

---

## Decision Record: Plugin vs Standalone Software

**Decision: WanaWorks stays a UE5 plugin for MVP and the foreseeable product. Do not fork into standalone software.**

Rationale:

1. **The value prop requires living inside the engine.** Everything WanaWorks does — Anim BP readiness, reflection writes onto live AnimInstances, WIT scans of level actors, sandbox working copies, Content Browser output assets — depends on the UE asset registry, reflection system, and live editor world. A standalone app would have to either reimplement engine internals (not feasible) or talk to a running editor over IPC anyway, at which point the plugin is still doing all the real work and standalone is just a second window.
2. **Product truth already says so.** AGENTS.md defines WanaWorks as "a premium software workspace *inside* UE5" and "a non-destructive orchestration layer over UE5-native systems." The premium-standalone *feel* is a UI/design goal (already achieved by the shell), not an architecture goal.
3. **MVP economics.** Standalone means installers, updaters, licensing, crash reporting, engine-version matrices, and a second codebase to keep alive. That is months of work that produces zero new demo value. Sealing an MVP argues for the smallest shippable surface, which is the plugin.
4. **Distribution.** Fab/Marketplace plugin distribution is an established channel with built-in discovery for exactly this audience. Standalone tooling for UE developers has no comparable channel.

**Escape hatch already in place:** the Remote Control plugin (enabled on this branch) gives a supported HTTP/WebSocket surface into the editor. If a standalone companion (dashboard, mobile monitor, CI hook) is ever wanted, it should be a *thin client* over Remote Control talking to the WanaWorks plugin — not a port of WanaWorks.

**Revisit only if:** a customer segment materializes that cannot run the UE editor at all, or WanaWorks pivots to multi-engine (the `EngineAdapterId` field in the Workflow Action Planner is the seam for that future, and it still would not require standalone software first).

---

## MVP Scope — What Is IN

The MVP is the current four-workspace product plus one visible-behavior gap closed:

1. Premium studio shell + workspace routing (Phases 1–2) — done, do not touch except bug fixes.
2. Live preview / studio stage (Phase 3) — done.
3. Autonomous subject workflow, Analyze / Enhance / Test / Build (Phase 4) — done.
4. Anim BP readiness + runtime WanaAnimation adapter (Phases 5/5A) — done.
5. Character Building workspace V1 (Phase 7) — done.
6. Level Design workspace V1 + WIT scan + persisted report (Phase 8) — done.
7. Workflow Action Planner + Project Health workspace (Phases 11A/11B) — done.
8. **Phase 6 — Visible Character/AI Improvement — the only MVP-blocking gap.** Impact → Physical State computes correctly but nothing visibly moves the mesh. This is the difference between an investor seeing a product and seeing a report generator.

## MVP Scope — What Is CUT (post-seal, explicitly deferred)

- Phase 9 (Assistant Generation), Phase 10 (WAI/WAMI depth), Phase 11 behavioral WIT consumption, Phase 12 (WanaCombat-lite), Phase 13 (WanaAnimation depth), Phase 14 (six future workspaces beyond honest "coming soon" states), Phase 15 (Synaptic Core).
- Historical action-plan resolution tracking (11A known limitation) — stays a documented limitation.
- Persistent Project Health report asset and top-bar health indicator (11B known limitations).
- Moving the WIT scan pipeline out of `WanaWorksUIModule.cpp` into `WanaWorksWIT` (Phase 8 known gap) — worthwhile refactor, not demo-visible, defer.
- Standalone/companion apps of any kind (see decision record).

Cutting these is what "sealing" means. Anything on this list proposed before seal should be rejected by default.

---

## Phase 6 Closure Plan (the remaining MVP work)

PLANS-2.md already identifies the practical path: a plugin-owned procedural effect layered on top of the existing Anim BP, driven by `StabilityScore` / `InstabilityAlpha` — because reflection auto-wire applies ~0 fields against the ALS AnimBP and editing the user's AnimGraph is prohibited.

**Step 1 — foundation component (added 2026-07-28, this branch):** `UWanaProceduralReactionComponent` in `WanaWorksCore` (`WanaProceduralReactionComponent.h/.cpp`). Reads the owner's `UWanaPhysicalStateComponent` every tick (editor ticking enabled, matching the physical state component) and applies a bounded lean toward the last impact direction, sway while Staggered/OffBalance/Panicked, a small vertical dip, and a smooth settle during Recovering. Non-destructive: only the mesh component's relative transform is offset, the base transform is captured and restored, and no animation asset is ever touched. `TriggerTestImpact()` gives Blueprints/editor a one-call demo hook.

**Step 2 — compile check (first session on a machine with UE 5.5):** this component was written in a remote session without the UE toolchain and has NOT been compile-verified. Compile before anything else; fix only the first error per AGENTS.md if it fails.

**Step 3 — wire into the workflow (small, scoped UIModule change):**
- Enhance (Character Intelligence): attach `UWanaProceduralReactionComponent` to the working-copy subject alongside the existing component attachment path.
- Test (Character Intelligence): where Test currently calls `ApplyImpactHint` / pushes text rows, the attached component now makes the same impact visibly move the mesh — verify medium vs strong impacts read differently, per Phase 6 exit criteria.
- Report the component's `bReactionActive` in the existing readiness detail string so the UI stays honest about whether the visible layer is live.

**Step 4 — verify Phase 6 exit criteria** in the editor: medium impact → visible disruption; stronger impact → stronger off-balance + recovery arc; enhanced subject visibly differs from baseline; observable without reading logs.

---

## Seal Checklist (maps to PLANS-2.md Demo-Ready Definition)

| # | Demo-ready requirement | Status |
|---|---|---|
| 1 | Premium software shell visible | Done |
| 2 | Subject picked from inside WanaWorks | Done |
| 3 | Stack auto-detected, no manual wiring | Done |
| 4 | Subject appears in live preview | Done |
| 5 | Enhance applies automatically | Done (add reaction component attach — Step 3) |
| 6 | Test shows visible behavior/animation/physical state result | **Blocked on Phase 6 Steps 2–4** |
| 7 | Build creates clean output in WanaWorks content folder | Done |
| 8 | Original source asset preserved | Done |
| 9 | UI investor-presentable | Done |
| 10 | P6™ loop explainable in a demo | Done |

**Seal = all 10 checked + one full end-to-end demo run (pick subject → Analyze → Enhance → Test with visible stagger → Build) recorded without touching the Output Log.**

## Post-seal order (unchanged from PLANS-2.md)

Phase 11 behavioral WIT consumption → Phase 9 → Phase 10 → Phase 12+.
