# WanaWorks Agent Instructions

## Product truth

WanaWorks is not just an AI plugin tab.

WanaWorks is a premium UE5 workspace platform and autonomous developer-assistance layer. It should feel like a standalone creative/development software experience that happens to live inside Unreal Engine.

WanaWorks must be treated as:
- a premium software workspace inside UE5
- a non-destructive orchestration layer over UE5-native systems
- a platform with multiple workspaces
- an autonomous assistant that detects, attaches, wires, enhances, tests, and builds where safely possible
- a product that hides complexity behind simple workflows

The user should not feel like they are using a utility panel.
The user should feel like they opened a professional WanaWorks studio environment.

## Core product philosophy

WanaWorks should follow this rule:

**The user selects what they want to work on. WanaWorks detects the rest, creates a safe working version, enhances it, tests it, and builds the final output.**

The default experience should be:
1. Choose subject
2. Enhance
3. Test / Analyze
4. Build

Do not expose unnecessary helper steps unless they are inside an advanced/developer area.

## Current priority order

When user direction conflicts with older local plans, follow the user's most recent clarified product vision.

Current priorities:

1. Premium software UI and visual identity
2. Real workspace switching
3. Live preview / studio viewport that actually shows the selected subject
4. Autonomous detection, attachment, wiring, enhancement, and build flow
5. Character Intelligence / AI workspace as the first flagship demo
6. Character Building workspace as a true sibling workspace
7. Level Design workspace as WIT-driven semantic world understanding
8. Visible improvement / wow behavior
9. UE5 assistant generation features
10. Broader WanaWorks platform expansion

## Platform workspaces

The shell should communicate that WanaWorks is a platform.

Visible workspace map:

- Character Intelligence
- Character Building
- Level Design
- Logic & Blueprints
- Physics
- Audio
- UI / UX
- Optimize
- Build & Deploy

Only Character Intelligence, Character Building, and Level Design need meaningful near-term surfaces right now.

Future workspaces may be present as polished coming-later or platform-extension lanes, but they must not confuse the user or pretend to be finished.

## Workspace definitions

### Character Intelligence

This is the flagship AI/NPC workspace.

It focuses on:
- AI Pawn selection
- Character Pawn AI-readiness
- AI stack detection
- WAI / WAMI identity, memory, emotion, role, and character model
- WAY-lite relationship and adaptation support
- WIT semantic environment support
- WanaAnimation hooks
- WanaCombat-lite response
- physical state
- behavior result
- automatic enhancement / test / build

This workspace should not feel like generic AI tools.
It should feel like a character-intelligence studio.

### Character Building

This is not a copy of Character Intelligence.

It focuses on:
- character subject selection
- character build context
- skeletal mesh / rig / animation awareness
- character profile and build-readiness
- identity and character-side relationship context
- character preview
- character-facing enhancement
- final character output

It should feel like a real workspace for building and preparing characters, not a renamed AI workspace.

### Level Design

This is the WIT/world understanding lane.

It focuses on:
- semantic environment meaning
- level context
- cover meaning
- obstacle meaning
- boundary and movement-space meaning
- scene tools
- environment analysis
- future level-building assistance

Scene and utility tools belong here, not in the Character Intelligence workspace.

## WIT / WAI / WAY definitions

### WIT = What Is This?

WIT is semantic world and environment understanding.

It should answer:
- What is this object?
- What is this space?
- What can AI do here?
- Is this cover?
- Is this an obstacle?
- Is this a boundary?
- Is this useful for combat, movement, stealth, or avoidance?

WIT belongs primarily to:
- Character Intelligence when AI needs world understanding
- Level Design when the user is working with scenes and environments

Do not surface WIT as random helper buttons. It should be a clean high-level scan/analyze capability.

### WAI / WAMI = Who Am I?

WAI/WAMI defines the AI's internal identity:
- role
- memory
- emotion
- personality
- state
- self-model
- long-term character evolution

WAI/WAMI belongs in Character Intelligence.

### WAY = Who Are You?

WAY defines how AI interprets another actor:
- player
- character
- target
- friend
- enemy
- acquaintance
- neutral

WAY supports Character Intelligence, but the relationship-context presentation also belongs naturally in Character Building when describing who a character is to AI.

## UI / UX rules

WanaWorks must not look like a default UE utility panel.

The UI should feel:
- premium
- clean
- cinematic
- modern
- minimal
- investor-ready
- branded
- software-like

Avoid:
- raw Unreal/Slate utility feel
- dense details-panel formatting
- too many thin outline boxes
- table-heavy row layouts
- button piles
- debug-console presentation
- output log as the primary feedback method
- generic editor tool styling

Prefer:
- custom WanaWorks styling
- strong hero preview area
- polished workspace rail
- premium cards
- status chips
- elegant typography hierarchy
- clear labels
- clean spacing
- minimal visible actions
- contextual automation

## Main workflow rule

The main visible workflow should stay centered around:

- Enhance
- Test
- Analyze
- Build

Do not reintroduce visible helper-button clutter.

The following should not be primary main-face buttons:
- Create Copy
- Use Selected as Observer
- Use Selected as Target
- Evaluate Working Pair
- Ensure Relationship
- Show Preset Summary
- Focus Observer
- Focus Target
- manual diagnostics
- old command/debug utilities

Those may exist only in a clearly secondary advanced/developer area if still needed.

Working-copy creation must happen automatically when required.

## Feedback / status rule

Do not rely on a bottom output log as the primary user feedback system.

Use:
- in-app success messages
- toast-style messages
- subtle status banners
- status chips
- result cards

Logs may exist in an advanced/developer area only.

## Build behavior rule

Build should be clean and non-intrusive.

Build must:
- preserve original source assets
- build final outputs into clear WanaWorks content folders
- avoid cluttering the active level viewport
- avoid stealing Content Browser focus unless explicitly requested
- show a quiet in-app success message with output path/location

## Preview / viewport rule

The center stage is the heart of the product.

The preview must:
- show the selected subject where practical
- not appear as a dead black placeholder in supported flows
- feel like a studio stage
- support view context such as Overview / Front / Back / Left / Right
- feel premium and alive

If a true live preview cannot be implemented in one pass, implement the strongest honest preview behavior possible and clearly indicate limitations.

Do not fake readiness. If a preview mode is not implemented, label it clearly and beautifully.

## Autonomous setup rule

The user should not manually wire systems if WanaWorks can do it safely.

WanaWorks should automatically:
- detect pawn type
- detect linked Anim BP
- detect AI controller
- detect skeletal mesh / rig context
- create working copy when needed
- attach required WanaWorks components to generated/working/finalized subjects
- auto-wire supported animation/behavior state
- preserve originals
- build final output cleanly

Manual setup instructions are a fallback only, not the primary product experience.

## Non-destructive rule

Never destroy, replace, or mutate original user assets as the primary path.

Prefer:
- generated working copy
- sandbox/workspace version
- finalized build output
- original preserved

WanaWorks enhances and orchestrates existing systems. It does not force project rewrites.

## Code architecture rules

- Work inside the WanaDemo project and WanaWorks plugin.
- Never edit Engine/ or Unreal fork source.
- Preserve compile-safe architecture.
- Runtime modules must remain runtime-safe.
- Editor/UI tooling currently lives in WanaWorksUI.
- Runtime modules may be used by the editor/UI module.
- Editor-only code must not leak into runtime dependencies.
- Dependencies must be intentional.

## Refactor freedom

Codex is allowed to refactor within the WanaWorks plugin when needed to support the current product direction.

Allowed when justified:
- extracting UI helper files
- creating style/theme helper files
- creating workspace builder helpers
- creating preview helper classes inside WanaWorksUI
- reorganizing presentation code
- replacing old visible UI structure
- deleting obsolete UI sections from the main face

Avoid:
- large backend rewrites without need
- new modules unless explicitly requested or clearly justified
- breaking existing build or workflow behavior

## Design-system freedom

Codex is encouraged to build a stronger WanaWorks UI design system.

Allowed:
- custom colors
- custom typography constants
- custom button styling
- custom card styling
- custom nav rail styling
- custom chip/status styling
- custom stage chrome
- custom empty states

Do not settle for default-looking Unreal utility styling.

## Workspace routing rule

The workspace rail must switch actual workspace bodies.

Clicking Character Intelligence, Character Building, and Level Design must visibly change the main content, not only active labels.

If a workspace is not fully implemented, show a polished and honest workspace-specific body, not an AI copy.

## Execution rules

- Execute one phase at a time unless user explicitly asks for a larger combined pass.
- After each phase, run UE build validation.
- Stop after the phase completes.
- If compile fails, stop on the first real error and fix only that.
- Do not widen scope beyond the user's current requested phase.
- Preserve existing working systems unless the user explicitly says to replace or hide them.

## Reporting contract

Every Codex result should include:

1. Files changed
2. Why they changed
3. Compile result
4. Manual validation steps
5. Suggested commit message

For UI work, also include:
6. What the user should visually check
7. Known limitations if any

## Screenshot test

For UI work, the pass is not successful unless a screenshot looks materially closer to a custom WanaWorks software product.

The screenshot should not read as:
- default Unreal tool
- debug dashboard
- plugin utility panel

It should read as:
- premium WanaWorks studio software
- autonomous workspace
- branded product

## Current product status

The prototype/demo foundation is roughly 70% complete.

The full WanaWorks vision is roughly 38-40% complete.

The biggest current blockers are:
1. premium custom visual identity
2. real workspace body switching
3. live preview / hero stage
4. stronger visible AI/character payoff
5. true UE5 assistant/generation layer
