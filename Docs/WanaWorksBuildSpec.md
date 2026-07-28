# WanaWorks Build Spec — How the Software Must Act

> Canonical behavior contract, v1 (2026-07-28). PLANS-2.md says *what to build in what order*; AGENTS.md says *how agents work*; this spec says **how the finished software must behave**. When implementation and this spec disagree, one of them is a bug — fix the code or amend the spec deliberately, never silently. Scope lock lives in `Docs/MVPSealPlan.md`.

---

## 1. Product Laws (non-negotiable, testable)

Every feature, present or future, must satisfy all seven. A feature that can't is out of scope by definition.

1. **Non-destructive, always.** Original assets are never mutated. All mutation targets are working copies or WanaWorks-owned outputs under `/Game/WanaWorks/...`. Deleting everything WanaWorks created must leave the project byte-identical to before.
2. **Honest UI.** Every capability claim in the UI is true at the moment it is shown. Unbuilt features say so ("Not Supported", "COMING LATER") — never a dead button, never a fake result. If a scan is heuristic, the UI may summarize confidence but must never imply ML/vision that doesn't exist.
3. **No log-reading.** The Output Log is never the feedback surface. Every outcome the user needs is visible in cards, toasts, or the stage. If a user must open the log to understand what happened, that flow is broken.
4. **One loop.** All workspace behavior is the P6™ loop (Perceive → Preserve → Prepare → Perform → Prove → Produce) surfaced as exactly four verbs: **Analyze / Enhance / Test / Build**. No workspace invents a fifth verb or hides one of the four.
5. **Autonomy first, manual as fallback.** On subject selection the stack is auto-detected (pawn type, mesh, Anim BP, controllers, component state, shared-asset risk). The user is never asked for something WanaWorks can detect. Manual wiring exists only where detection is genuinely impossible, and the UI says why.
6. **Degrade, never break.** Every capability resolves to exactly one of four readiness states: **Full → Limited → Needs Enhance → Not Supported**. Missing prerequisites step down this ladder with a stated reason; they never throw, stall, or silently no-op.
7. **Premium presence.** WanaWorks looks and behaves like standalone creative software living inside UE (see §6). It never steals Content Browser focus, never spawns clutter in the user's viewport, never opens modal dialogs for things a card can say.

---

## 2. The Golden Path (the workflow, perfected)

This is the experience the whole product is judged by. Timing budgets are contracts, not aspirations.

| Step | User action | System behavior | Budget |
|---|---|---|---|
| 0 | Opens WanaWorks tab | Shell renders complete (rail, stage, cards, action strip). No placeholder flicker, no empty panels. | < 1 s to interactive |
| 1 | Clicks a workspace | Body widget actually changes (routing law). Stage + cards + action plan swap to that workspace's context. | < 100 ms |
| 2 | Picks a subject (in-workspace picker) | Auto-detect runs: pawn/character class, skeletal mesh, Anim BP, AI/Player controller, existing Wana components, shared-stack risk. Results land in status cards. Subject appears in the preview stage where supported. | < 2 s to first card |
| 3 | **Analyze** | Perceive + diagnose. Readiness per capability (ladder states), top-5 Next Actions from the Workflow Action Planner, cross-workspace pointers (e.g. "Run Level Design Build" when the WIT report is missing). Read-only — Analyze never mutates anything, ever. | < 3 s |
| 4 | **Enhance** | Preserve + Prepare + Perform: create/refresh working copy, attach Wana components (physical state, adapter, procedural reaction), wire what is safely wireable, refresh readiness. Idempotent — Enhance twice yields the same state, never duplicate components. | < 5 s, progress visible if longer |
| 5 | **Test** | Prove: drive a real, *visible* result on the working-copy subject — impact → stagger/lean/recovery arc on the mesh (Phase 6), state values in cards updating live. Medium and strong impacts must be visually distinguishable. Never moves or mutates the original subject. | reaction starts < 250 ms after click |
| 6 | **Build** | Produce: clean, inspectable output (report assets, adapter assets) under `/Game/WanaWorks/<Category>/`. A completion card names every asset produced with its path. Quiet: no viewport spawns, no Content Browser focus steal. | < 5 s, progress visible if longer |
| 7 | Inspects results | Everything Build claimed exists, opens, and is editable. Original asset untouched (verifiable by diff/timestamp). | — |

**Interruption rule:** every step is abandonable. Switching workspace or subject mid-flow cancels cleanly; nothing half-applied remains on any original asset (working copies may be left in any state — they're disposable by definition).

**Repeat rule:** the loop is re-entrant. Analyze → Enhance → Test → Test → Analyze → Build in any order never corrupts state; verbs check current reality, not remembered assumptions.

---

## 3. Behavior Contract per Verb

For every workspace, each verb obeys this contract. Workspace sections (§4) only add specifics — they cannot subtract guarantees.

### Analyze
- **Mutates:** nothing. Guaranteed read-only.
- **Always produces:** readiness ladder per capability + prioritized action plan (max 5 surfaced) + honest summary text.
- **Failure mode:** a capability that can't be evaluated reports Not Supported with reason — Analyze itself cannot fail; a subjectless workspace analyzes what it can (Level Design and Project Health need no subject).

### Enhance
- **Mutates:** working copies and WanaWorks-owned assets only.
- **Idempotent:** re-running converges to the same state (component attach checks for existing instance first; never stacks duplicates).
- **Transparent:** completion card lists exactly what was attached/wired/skipped and why (skips cite the ladder state).
- **Never:** edits user Anim BP graphs, Behavior Trees, Blackboards, State Trees, or replaces assigned Anim Classes without explicit user approval through a supported path.

### Test
- **Proves, visibly.** A test that only writes text rows is a broken test (Phase 6 law). The minimum bar: a non-technical viewer sees the difference between untested and tested, and between medium and strong stimulus, without explanation.
- **Targets working copies only.** The user's original subject never moves.
- **Reversible:** the subject returns to its base state after the arc completes (base-transform restore is part of the contract — see `UWanaProceduralReactionComponent`).

### Build
- **Outputs are assets**, not side effects: inspectable, editable, saved under WanaWorks output folders, named predictably (`<SubjectName>_<ReportType>` pattern).
- **Repeatable:** rebuilding overwrites WanaWorks-owned outputs cleanly or versions them — never errors on "already exists", never litters.
- **Reports its own honesty:** the completion card includes what was *not* built and why, using ladder language.

---

## 4. Per-Workspace Behavior

### Character Intelligence
- Focus: AI subject — WAI/WAMI, WAY influence, WIT context, physical state, behavior readiness.
- Must: auto-detect the AI stack; surface the full adapter field set (posture/reaction/behavior/fallback/instability/recovery); fold WIT report context into recommendations; drive the visible Test reaction (Phase 6).
- Must not: contain level-design tools or character-authoring tools; recommend actions the planner can't justify.

### Character Building
- Focus: character as an asset — skeletal/rig/anim readiness, identity, playable readiness, build output.
- Must: report shared Character BP / mesh / skeleton / Anim BP risk before any Enhance touches a shared stack; show playable-controller readiness; produce build-readiness output.
- Must not: silently enhance a shared stack (shared risk downgrades to Needs Enhance with an explanation instead).

### Level Design
- Focus: semantic world, no subject concept. WIT scan → Cover / Obstacle / MovementSpace / Boundary / NavigationRelevant classification → live cards → persisted `WanaWITEnvironmentReportAsset`.
- Must: keep "Full Level Generation" honestly Not Supported until real; scan bounded (no unbounded all-actors iteration on huge levels without a progress surface).
- Must not: contain AI-pawn pickers, WAI cards, or character tools.

### Project Health
- Focus: read-only project diagnosis — engine version, modules, plugin dependencies, editor-only-dep leaks in Runtime modules, WanaWorks output readiness.
- Must: stay read-only forever (diagnosis and guidance, zero auto-repair); reuse the shared analysis/action-plan pipeline (no parallel systems).
- Must not: acquire a subject/preview concept.

### Future workspaces (Logic & Blueprints, Physics, Audio, UI/UX, Optimize, Build & Deploy)
- Render as polished, honest coming-soon states that name what they will do. Never blank, never clones of Character Intelligence, never interactive-looking.

---

## 5. State, Feedback, and Failure Model

- **Single source of truth per session:** current workspace, current subject, latest analysis summary, latest action plan. Verbs read fresh reality; nothing trusts a stale snapshot across subject/workspace switches.
- **Feedback surfaces, in order of preference:** stage (visible behavior) → status cards → toast strip. Never modal dialogs for status, never the Output Log (log entries may exist for developers but carry nothing exclusive).
- **The action plan is the narrator.** At any moment, the top of the plan answers "what should I do next and why" — including "nothing; you're demo-ready." An empty plan is itself a rendered, positive state.
- **Failure grammar:** every failure message = *what was attempted + why it stepped down the ladder + the next action*. ("Anim BP auto-wire applied 0 fields — target Anim BP exposes no matching properties. Visible reactions use the procedural layer instead. No action needed.") Blame states, never the user.
- **No historical memory claims (current limitation, honored):** the planner reports current state only. Until resolution tracking exists, no UI text may claim "fixed since last run".

---

## 6. Feel & Craft Bar (what "premium" means concretely)

- All primary surfaces styled through `WanaWorksUIStyle` (named brushes, named `FSlateFontInfo`, named colors). Zero default-toolbar styling, zero raw `SDetailsView`/table-row bodies, zero inline magic point sizes.
- Layout invariant: left rail / center hero stage / right status cards / bottom strip with at most Enhance · Test · Analyze · Build.
- Motion: state changes animate (card refresh, stage reaction); nothing pops. The Test reaction is the hero moment — it must read as *crafted*, with sharp onset (ReactionInterpSpeed) and gradual settle (RecoveryInterpSpeed), never robotic snapping.
- Language: calm, specific, product-voiced. No exclamation marks, no dev jargon in user-facing strings ("reflection FProperty write" belongs in docs, not cards).
- Never: steal focus, spawn into the user's viewport uninvited, block the editor thread long enough to drop frames during scans (long work is chunked or async with visible progress).

---

## 7. Performance Budgets

| Concern | Budget |
|---|---|
| Tab open → interactive | < 1 s |
| Workspace switch | < 100 ms |
| Subject auto-detect first feedback | < 2 s |
| Analyze / Enhance / Build completion (typical subject/level) | < 3–5 s, progress surface beyond that |
| Test visible onset | < 250 ms |
| Per-tick cost of Wana runtime components (physical state, procedural reaction, adapter) combined | < 0.1 ms per subject; components tick-disable when settled |
| Editor frame drops caused by WanaWorks UI refresh | none perceptible (chunk scans > ~1000 actors) |

---

## 8. Definition of Perfect — Acceptance Scenarios

Sealed = all scenarios pass in a live editor, unrehearsed, without touching the Output Log.

1. **Cold open:** fresh editor → open WanaWorks → shell complete and interactive < 1 s; all four workspaces route to distinct bodies.
2. **Golden path:** pick ALS-based AI subject → Analyze (honest readiness incl. "auto-wire 0 fields" explanation) → Enhance (components attached, working copy created, original untouched) → Test (visible stagger; strong impact visibly stronger; clean recovery to exact base pose) → Build (named assets exist at stated paths).
3. **Idempotency:** run Enhance three times → component count and state identical to once.
4. **Shared-stack safety:** subject sharing an Anim BP with another character → Enhance downgrades the risky part with a card explaining the risk, mutates nothing shared.
5. **Subjectless workspaces:** Level Design scan + Build produce the WIT report with no subject selected; Project Health analyzes with nothing selected at all.
6. **Interruption:** switch workspace mid-Enhance → no partial mutation on any original; return → Analyze reports true current state.
7. **Cross-workspace intelligence:** delete the WIT report → Character Intelligence's plan tells you to run Level Design Build; run it → recommendation clears on next Analyze.
8. **Honesty sweep:** click every visible control in all four workspaces + every coming-soon workspace → zero dead controls, zero fake results, zero log-only outcomes.
9. **Cleanup:** delete `/Game/WanaWorks/` + remove Wana components → project diffs clean against pre-WanaWorks state.
10. **The demo:** steps 1–9 performable as one continuous 5-minute investor demo by a non-developer following only on-screen guidance.

---

## 9. Spec → Implementation Map (current truth)

| Spec section | Implemented by | Status |
|---|---|---|
| Shell, routing, stage, strip (§2 steps 0–2, §6) | `WanaWorksUIStyle`, `WanaWorksUITabBuilder` | Done |
| Analyze/action plan (§3, §5) | `BuildCharacterIntelligence/CharacterBuilding/LevelDesign/ProjectHealthWorkflowActionPlan`, `FWanaWorkflowActionPlanItem` | Done |
| Enhance component attach (§3) | existing attach path in `WanaWorksUIModule.cpp` | Done; add `UWanaProceduralReactionComponent` attach (MVPSealPlan Step 3) |
| Visible Test (§2 step 5, §3) | `UWanaPhysicalStateComponent` + `UWanaProceduralReactionComponent` | Component authored; compile + wire = the one open MVP item |
| Build outputs (§3) | `WanaWITEnvironmentReportAsset`, `WanaAnimationAdapterReportAsset`, output folders | Done |
| Ladder degradation (§1.6) | readiness states across workspaces | Done |
| WIT scan + honesty (§4) | `RunWITEnvironmentScan`, hard-coded honest Not Supported | Done |
| Project Health read-only (§4) | `WanaWorksProjectHealthActions` | Done |
| Budgets (§7) | not yet measured | Measure during seal pass; fix only real violations |
| Acceptance scenarios (§8) | manual seal checklist | Run at seal (MVPSealPlan) |

**Lead-dev sequencing note:** nothing in this spec licenses new systems before seal. The order is fixed: compile the Phase 6 component → wire Enhance/Test → run §8 → seal. Everything else in this spec is either already true or a measurement task.
