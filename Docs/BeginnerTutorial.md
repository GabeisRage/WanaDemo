# WanaWorks Beginner Tutorial

## What this is

This guide is for beginners who want to try the current `WanaWorks` systems inside `WanaDemo` without needing to understand the whole plugin first.

Right now, the easiest way to think about the plugin is:

- `WIT` helps classify what an actor seems to be
- `WAI` stores simple memory
- `Identity` stores who an actor is
- `WAY` stores how one actor feels about another actor

## Before you start

Make sure:

- `WanaDemo` opens in Unreal Editor
- the `Wana Works` tab is visible
- you have at least two actors in the level if you want to test observer and target relationships

If the tab is not already open, look for the `Wana Works` editor tab after the plugin loads.

## Step 1: Open the Wana Works tab

When the project opens, the plugin registers a dockable editor tab named `Wana Works`.

Inside the tab you will see several sections, including:

- `Weather`
- `WanaAI`
- `Scene`
- `Advanced Command`
- `Output`

Inside `WanaAI`, the main sections are:

- `WIT`
- `WAI`
- `Identity`
- `WAY`

## Step 2: Learn the selection rule

The current WAY workflow uses a very simple selection model:

- the first selected actor is the `Observer`
- the second selected actor is the `Target`
- if you only select one actor, the target falls back to the observer

This matters because WAY relationships are owned by the observer and point toward the target.

Example:

- `NPC_Guard` can be the observer
- `BP_PlayerCharacter` can be the target

That means the stored relationship is:

`NPC_Guard -> BP_PlayerCharacter`

## Step 3: Try the Scene tools

Start with something simple:

1. Open the `Scene` section.
2. Click `Spawn Cube`.
3. Click `List Selection`.

This helps confirm the tab is working and the output log is updating correctly.

## Step 4: Add Identity to an actor

Identity is lightweight actor metadata. It does not run heavy AI logic.

To test it:

1. Select an actor in the level.
2. Open the `Identity` section.
3. Click `Ensure Identity Component`.
4. Enter a `Faction Tag`.
5. Pick a `Default Relationship Seed`.
6. Click `Apply Identity`.

What this does:

- adds `UWanaIdentityComponent` if it is missing
- stores the actor's faction tag
- stores the default relationship seed that WAY can use later

The summary panel will show:

- selected actor
- whether identity exists
- faction tag
- default relationship seed
- reputation tag summary

## Step 5: Create a simple WAY relationship

This is the most important beginner workflow.

### One-actor fallback test

1. Select one actor.
2. Open the `WAY` section.
3. Click `Ensure Relationship Profile`.
4. Click `Show Relationship Profile`.

Because only one actor is selected, the target falls back to the observer. This is useful for debugging, but the more realistic workflow uses two actors.

### Two-actor observer -> target test

1. Select your observer actor first.
2. Ctrl-select your target actor second.
3. Check the summary at the top of the `WAY` section.

It should clearly show:

- `Observer`
- `Target`

Then:

1. Choose a relationship state in the dropdown.
2. Click `Ensure Relationship Profile`.
3. Click `Apply Relationship State`.
4. Click `Show Relationship Profile`.

The output log should show:

- observer
- target
- relationship state
- reaction
- trust
- fear
- respect
- attachment
- hostility

## Step 6: Understand reaction mapping

WAY currently uses a simple deterministic reaction layer.

The current mapping is:

- `Enemy -> Hostile`
- `Friend -> Cooperative`
- `Partner -> Protective`
- `Acquaintance -> Cautious`
- `Neutral -> Observational`

This is intentionally simple. It gives you a readable result that existing Character Blueprints, AI logic, or future systems can build on.

## Step 7: Use WIT classification

`WIT` is a lightweight classifier for selected actors.

To test it:

1. Select an actor.
2. In the `WIT` section, click `Classify Selected`.

The output will show a readable profile such as:

- actor name
- class
- tags
- components
- classification tag
- domain
- confidence

This is useful for quick inspection and future observer logic.

## Step 8: Use WAI memory

`WAI` currently stores simple memory entries.

To test it:

1. Select an actor.
2. In the `WAI` section, click `Add Memory Test`.
3. Click `Show Memory`.

This will add a small memory record and print it in the output area.

## Step 9: Call WAY from Blueprint

The editor tab is useful for testing, but the runtime-friendly path is Blueprint.

### Observer setup

Add `UWAYPlayerProfileComponent` to the observer actor.

### Target setup

Add `UWanaIdentityComponent` to the target actor and set its `DefaultRelationshipSeed`.

### Blueprint call

Call:

`EvaluateTarget(TargetActor)`

on the observer's WAY component.

This does three things:

- ensures a relationship profile exists
- seeds it from the target identity when available
- returns the current reaction

You can also read:

- `GetReactionForTarget(TargetActor)`
- `GetRelationshipProfileForTarget(TargetActor)`
- `EvaluateAndGetReaction(TargetActor)`

## Step 9.5: Two easy Blueprint bridge patterns

If you already have an NPC Blueprint, Character Blueprint, or AI Controller, these are the easiest non-destructive patterns to start with.

### Pattern A: One-node reaction check

Use:

`EvaluateAndGetReaction(TargetActor)`

This is the quickest beginner-friendly option when you just want a stable current reaction for branching logic.

Example use:

- call it on `BeginPlay`
- call it during a simple perception callback
- call it during an overlap or interaction event
- switch on the returned `EWAYReactionState`

### Pattern B: Full evaluation result

Use:

`EvaluateTarget(TargetActor)`

This is better when you want both:

- the reaction
- the full relationship profile

It is useful if you want to inspect trust, fear, respect, attachment, or hostility in addition to the final reaction.

## Step 10: React to changes

The WAY component now exposes a lightweight reaction-change event:

`OnReactionChanged`

This event gives you:

- observer
- target
- relationship state
- reaction state

This is the safest beginner-friendly way to layer WAY into an existing character without replacing your current systems.

Good uses:

- print debug text
- update a Blackboard key from Blueprint
- drive a simple state enum on a character
- branch existing logic based on reaction

## Suggested first experiment

If you want one clean beginner exercise, try this:

1. Place two actors in the level: one observer and one target.
2. Give the target an identity with `Default Relationship Seed = Friend`.
3. Give the observer a WAY component.
4. In Blueprint, call `EvaluateTarget(TargetActor)` on BeginPlay.
5. Print the returned reaction.

Expected result:

- the observer creates a profile for the target
- the target seed sets the relationship state
- the returned reaction becomes `Cooperative`

Then change the target seed to `Enemy` and test again.

The returned reaction should become `Hostile`.

## Troubleshooting

### The WAY summary shows the wrong actor

Check selection order:

- first selected actor = observer
- second selected actor = target

### The target falls back to the observer

That means only one actor is selected. Select a second actor if you want a separate target.

### No relationship appears

Make sure the observer has `UWAYPlayerProfileComponent`.

### The seed does not seem to apply

Make sure the target has `UWanaIdentityComponent` and that `DefaultRelationshipSeed` is set before evaluation.

### Nothing prints in the output area

Make sure the `Wana Works` tab is open and that you clicked a button or ran a command that produces output.

## What to learn next

After you are comfortable with this tutorial, the next things worth learning are:

- how observer and target selection works in the `WAY` section
- how to bind `OnReactionChanged` in Blueprint
- how to use `EvaluateTarget` from an existing NPC Blueprint
- how to use WIT and WAY together for simple AI evaluation flows
