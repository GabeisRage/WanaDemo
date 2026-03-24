# WanaWorks phased autopilot plan

## Phase A - Stabilize current shell
- Preserve current WanaWorks tab and command loop
- Keep help, clear, echo working
Exit criteria:
- UE build passes
- Tab opens
- Commands respond

## Phase B - Editor-world commands
- Add spawn cube
- Add list selection
Exit criteria:
- Cube spawns in editor world
- Selected actor names are listed
- UE build passes

## Phase C - WIT selected classification
- Add classify selected using simple heuristics
Exit criteria:
- Selected actor returns readable profile
- UE build passes

## Phase D - WAI/WAY visibility
- Add test commands to populate memory and preference data
- Surface state in the tab
Exit criteria:
- State is visible and changes on command
- UE build passes

## Phase E - Render controls
- Add one or two safe scene look commands
Exit criteria:
- Scene visibly changes
- UE build passes

## Phase F - Weather prototype
- Add a first weather control path
Exit criteria:
- Weather preset visibly changes environment
- UE build passes

## Phase G - Python bridge
- Connect one command to wana_ai_hooks.py
Exit criteria:
- Command roundtrip succeeds
- UE build passes
