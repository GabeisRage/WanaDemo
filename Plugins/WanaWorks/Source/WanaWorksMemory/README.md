# WanaWorksMemory

Studio-owned memory for WanaWorks characters. Conversation turns, relationship scores, and WIT / WAY / WAI / WAMI identity live in a SQLite file under the project's `Saved/` directory. The language model is only a stateless text generator. Swapping xAI Grok for OpenAI ChatGPT does not move or copy that database.

This is a separate runtime module. It does not change Core, WIT, WAY, WAI, UI, or Render. It is **not registered** in `WanaWorks.uplugin` yet. See [Enabling the module](#enabling-the-module).

## Ownership

| What | Where it lives |
|---|---|
| Turns, memories, relationship scores, identity JSON | `Saved/WanaWorks/Memory/wana_memory.db` |
| Which provider and model to call | `Saved/WanaWorks/Memory/provider.json` (no keys) |
| API key | Environment variable, or `Saved/WanaWorks/Memory/provider.secrets.json` |
| Vendor account | Nothing. No assistants, threads, files, or stored-response APIs are called |

A completion request does send the assembled prompt for that one call. The prompt is not written to a vendor memory store. The module never sends `store: true`, a vendor `user` id, or an embeddings request. Player and character ids are not used as vendor account keys.

`UWanaSettings::OpenAIApiKey` is a `config=Editor`, `defaultconfig` property. Unreal writes that into `Config/DefaultEditor.ini`, which is easy to commit. **This module does not read that property.** Do not put provider keys there.

On this branch the root `.gitignore` still contains unresolved conflict markers, so `Saved/` is not reliably ignored here. `master` commit `eb453e0` fixes that file. Do not `git add` `Saved/` or `provider.secrets.json`. This module does not edit `.gitignore`, so the fix on `master` can land on its own.

## Call flow

`Talk to Character` on `UWanaMemorySubsystem` (or the same node on `UWanaMemoryLibrary`):

1. **Perceive.** Load recent turns, top-K salient memories, relationship scores, the latest relationship change, and WIT / WAY / WAI / WAMI JSON for that character + player pair.
2. **Preserve.** Read-only against project assets. The only write target is the WanaWorks database under `Saved/`.
3. **Prepare.** Build a system prompt plus chat messages inside a character budget (default 8000 characters, about 2000 tokens at 4 characters per token). Lowest priority is dropped first: WIT, then WAY, then WAI, then WAMI, then the weakest memories, then the oldest turns. The preamble, current scores, and the new player line stay.
4. **Perform.** `POST` that prompt to `https://api.x.ai/v1/chat/completions` or `https://api.openai.com/v1/chat/completions`. `IHttpRequest::ProcessRequest` returns immediately. The game thread is not blocked. `ProcessRequestUntilComplete` is not used.
5. **Prove.** The reply is parsed. If the model omits a relationship delta, a small local phrase heuristic may apply a capped change. The spoken reply is what Blueprint receives. Warnings stay on `Get Last Status`, not in the spoken line.
6. **Produce.** The user turn is stored before the request. After a successful reply the module stores the assistant turn (tagged with the provider id), a salient memory, and any relationship change plus a history row.

A failed request leaves the user turn in the transcript and does not invent an assistant line.

The model is asked to end with:

```text
<wana_memory>
{"memory":"one short sentence","importance":0.5,"relationship_delta":{"trust":0,"affinity":0,"fear":0,"respect":0}}
</wana_memory>
```

That block is removed before the player hears the reply. Model deltas are clamped to ±0.25 per axis per turn. Scores themselves stay in the range 0 to 1. Defaults before any row exists are trust 0.50, affinity 0.50, fear 0.00, respect 0.50.

## WAMI

WAMI is stored as its own identity record, next to WIT, WAY, and WAI. The assumption in this module: **WAMI is the character's durable self-model** (faction, reputation, role, values, the "who I am" layer Miles is naming). WAI remains the runtime personality / emotion / event layer. The module does not invent gameplay from WAMI. It stores a versioned JSON blob and injects that blob into the prompt.

The legacy adapter, when asked, copies `UWanaIdentityComponent` (faction, reputation tags, default relationship seed) into the WAMI blob. That is a convenience mapping, not a claim that the component was designed as WAMI.

WIT has no component adapter. Call `Set Identity State` with kind `WIT` and whatever JSON the scan produced. Unknown extra JSON fields are stored as-is. The blob's `schema_version` is yours. It is not the database schema version.

## Schema

SQLite `user_version` is the database schema. Current version is **2**.

| Version | Change |
|---|---|
| 1 | `schema_migrations`, `conversation_turns`, `relationship_scores`, `relationship_history`, `identity_state`, `salient_memories` |
| 2 | `conversation_turns.provider_id` |

Opening a database applies any missing migrations inside a transaction and records them in `schema_migrations`. A database newer than this build is refused and left untouched.

Rows are keyed by `character_id` + `player_id` (identity rows by `character_id` + `state_kind`). Kinds are normalized to uppercase. `WIT`, `WAY`, `WAI`, and `WAMI` are the canonical kinds. Other letter/digit/`_` kinds are accepted so the set can grow without a migration.

`salient_memories.embedding` is an optional native float32 blob used only by a local embedding provider. It is null unless embeddings are turned on.

## Providers

`IWanaLLMProvider::CompleteAsync` is the only model call. Adapters:

| Config `provider` | Endpoint | Key environment variables, in order |
|---|---|---|
| `grok` or `xai` | `https://api.x.ai/v1/chat/completions` | `WANA_XAI_API_KEY`, then `XAI_API_KEY`, then `xai_api_key` in the secrets file |
| `openai` or `chatgpt` | `https://api.openai.com/v1/chat/completions` | `WANA_OPENAI_API_KEY`, then `OPENAI_API_KEY`, then `openai_api_key` in the secrets file |

Environment variables win over the secrets file. Copy `Config/provider.example.json` to `Saved/WanaWorks/Memory/provider.json` to change provider, models, temperature, token budget, or embeddings. Key fields in that file are ignored and are never written back.

Secrets file shape:

```json
{
  "xai_api_key": "paste-locally-only",
  "openai_api_key": "paste-locally-only"
}
```

Blueprint `Set Active Provider` (`grok` or `openai`) and `Set Model Override` rewrite `provider.json` without key fields. They do not touch `DefaultEditor.ini`.

`endpoint_override` replaces the URL. A URL that contains `api_key=`, `apikey=`, or `access_token=` is rejected.

Default models are `grok-4` and `gpt-4o-mini`. Override them in config when the account uses a different name.

## Embeddings

`IEmbeddingProvider` is optional. The default is off, which means keyword overlap plus importance. `embeddings: "local-hash"` turns on a 64-dimension local hasher. Vectors stay in the studio database. There is no client for a vendor embeddings API. A future provider must run locally.

## Wiring a character

1. Add the module entry below and compile in Unreal 5.5.
2. Pick stable ids. A character Blueprint name and a player profile id are enough. The same pair must be used for every conversation you want remembered.
3. On BeginPlay, or from an editor utility, call `Set Identity State` for `WAMI` (and `WAI`, `WAY`, `WIT` if you have JSON for them).
4. Call `Talk to Character` with those ids and the player's line. Bind the reply delegate. Show that string in your own UI. The output log is not the result.
5. Optional, and only when you call them: `Save Legacy Snapshot` and `Load Legacy Snapshot` copy `UWAIMemoryComponent`, `UWAIEmotionComponent`, `UWAIPersonalityComponent`, `UWAYPlayerProfileComponent`, and `UWanaIdentityComponent` through the store. They do not run on their own, and they do not change those components' existing behavior. `Import Relationship From WAY` is a separate call. WAY has attachment and hostility rather than affinity, so affinity is `attachment * (1 - hostility)`, clamped to 0..1.

`Get Transcript`, `Get Relationship History Text`, and `Get Provider Summary` are for designers inspecting state. `Get Provider Summary` says whether a key is set. It does not print the key.

## Enabling the module

The Unreal 5.5 toolchain is not available in the environment that added this module, so `WanaWorks.uplugin` was left unchanged. A module listed there is compiled with the rest of the plugin. An untested compile error would break the existing build.

After a local compile succeeds, add this object to the `Modules` array in `Plugins/WanaWorks/WanaWorks.uplugin`:

```json
{
  "Name": "WanaWorksMemory",
  "Type": "Runtime",
  "LoadingPhase": "Default"
}
```

No `.uproject` change is required. The engine `HTTP` module is linked by the build file. The engine SQLiteCore / SQLiteSupport plugins are not used. SQLite 3.53.4 is vendored as `ThirdParty/sqlite/sqlite3.c.inc` (public domain, SHA3-256 `67f423e9ebbbdc473cbc4772c872ee6b89f31fde4ed0279a5c25d5f65c043a16`). Its symbols are hidden on GCC/Clang so they are less likely to collide with SQLiteCore if that plugin is also loaded.

## Tests

Core logic (store, migrations, retrieval, prompt budget, provider JSON, secret handling, legacy snapshot JSON, local embeddings, and the talk flow against a scripted provider) compiles without Unreal:

```bash
make -C Plugins/WanaWorks/Source/WanaWorksMemory/Tests test
```

That command uses `g++ -std=c++17 -Wall -Wextra -Werror -fno-exceptions -fno-rtti`, which is close to an Unreal runtime TU. Last run: **182 passed, 0 failed**.

Not compiled and not run here:

- Unreal Build Tool, the HTTP adapter, the game-instance subsystem, and the legacy component adapter
- A live call to api.x.ai or api.openai.com

## Layout

```text
WanaWorksMemory.Build.cs          Unreal module rules. Not loaded until the uplugin entry exists.
Portable/                         Standard C++. No Engine headers. This is what the tests compile.
ThirdParty/sqlite/                SQLite amalgamation.
Public/ Private/                  Unreal subsystem, HTTP provider, legacy adapter.
Config/provider.example.json      Provider selection only. No keys.
Tests/                            Standalone runner.
```

## Risks

- The Unreal sources have not been compiled. Likely follow-ups on a real 5.5 build are `MakeShared` template arguments, `SetTimeout`, or a reflected property name on the legacy adapter. None of that is in the current plugin build, because the module is unregistered.
- The legacy load path writes private `UPROPERTY` arrays by name (`MemoryBank`, `Memories`, `RelationshipProfiles`). If those names change, load leaves the component alone and says so in the report.
- Retrieved text is marked as data in the system prompt. That is not a guarantee against prompt injection.
- The phrase heuristic is coarse and capped at ±0.08. An explicit model delta of zero suppresses it.
- The vendored amalgamation is large. It is source, not a binary asset, and it is marked `linguist-vendored`.
