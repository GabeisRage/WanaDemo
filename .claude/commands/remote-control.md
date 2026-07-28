---
description: Work with Unreal's Remote Control API in this project — verify setup, start the servers, and send editor commands over HTTP/WebSocket
---

Help with Unreal Engine Remote Control for this project. Task from the user (may be empty): $ARGUMENTS

## Project setup (already in the repo)

- The engine's **Remote Control** plugin is enabled in `WanaDemo.uproject`.
- `Config/DefaultRemoteControl.ini` auto-starts both servers when the editor launches:
  - HTTP API on port **30010**
  - WebSocket API on port **30020**

If the user reports the servers are not running, have them check **Project Settings → Plugins → Remote Control** in the editor, or start the servers manually from the editor console: `WebControl.StartServer` and `WebControl.EnableServerOnStartup`.

## Important constraint

The Unreal Editor runs on the user's machine, not in this remote session. This session can write code, config, and presets, and can compose HTTP requests for the user to run — but it cannot reach `localhost:30010` on the user's machine. When a live editor call is needed, give the user the exact `curl` command (or PowerShell `Invoke-RestMethod`) to run locally.

## Common HTTP API calls

All requests go to `http://localhost:30010`, JSON body, `Content-Type: application/json`.

Check the API is up:

```
curl http://localhost:30010/remote/info
```

Call a function on an object (example — save all dirty packages):

```
curl -X PUT http://localhost:30010/remote/object/call \
  -H "Content-Type: application/json" \
  -d '{
    "objectPath": "/Script/EditorScriptingUtilities.Default__EditorAssetLibrary",
    "functionName": "SaveLoadedAssets"
  }'
```

Read or set a property on an actor:

```
curl -X PUT http://localhost:30010/remote/object/property \
  -H "Content-Type: application/json" \
  -d '{
    "objectPath": "<full object path, e.g. /Game/NewMap.NewMap:PersistentLevel.SomeActor>",
    "access": "READ_ACCESS"
  }'
```

Use `"access": "WRITE_ACCESS"` plus `"propertyValue": { ... }` to set values. Search for objects with `PUT /remote/search/assets`. Remote Control Presets are exposed under `/remote/preset/...`.

## When invoked

1. If `$ARGUMENTS` describes a task (e.g. "move the ALS character", "expose the sun angle"), figure out the object paths involved from the project source/content, then produce the exact Remote Control request(s) that accomplish it, ready for the user to run locally.
2. If no task was given, confirm the setup above is intact (`WanaDemo.uproject` has the `RemoteControl` plugin entry, `Config/DefaultRemoteControl.ini` exists) and summarize how to use the API.
3. Respect this repo's rules in `AGENTS.md`: never compose destructive calls against original assets — WanaWorks workflows operate on working copies.
