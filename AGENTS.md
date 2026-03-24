# WanaWorks agent instructions

# Current architecture override

- WanaWorksUI is currently the editor-facing module and owns editor tab registration.
- Do not introduce a new WanaWorksEditor module unless explicitly requested.
- Preserve the current compile-safe architecture and build forward from it.
- Prefer minimal, incremental changes over structural refactors.

# WanaWorks autopilot rules

Scope:
- Work only inside the WanaDemo project and the WanaWorks plugin.
- Never edit Engine/ or any Unreal fork source.
- Follow PLANS.md in order.

Architecture rules:
- Runtime modules remain runtime-safe.
- WanaWorksUI currently owns editor UI and editor tooling.
- Runtime modules may be used by the editor module.
- Editor modules must not become dependencies of runtime modules.
- Keep dependencies minimal and intentional.

Execution:
- Execute one phase at a time.
- After each phase, run UE build validation.
- Stop after the phase completes.
- If compile fails, stop on the first real error and fix only that.
- Do not create new modules unless explicitly requested.
- Do not widen scope beyond the current phase.
- Preserve the existing compile-safe architecture.

Reporting contract:
1. Files changed
2. Why they changed
3. Compile result
4. Manual validation steps
5. Suggested commit message
