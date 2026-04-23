# Implementationplan

This plan is based on the TowerFall "How To Play" mechanics page:
http://towerfall.wikidot.com/how-to-play

It also reflects the current state of this repository as of this review. The project already has a fight scene, level loading, archer rendering, input polling, basic collision, and some player movement. It does not yet have the real TowerFall combat loop.

## Current implementation snapshot

The fight state currently contains only the active level index and archers. `src/fight/scene.hpp:16` still has placeholders for arrows, chests, and effects.

`src/fight/archer.hpp:10` stores only position, velocity, facing, alive, crouching, and a fixed hitbox. There is no explicit movement state, dodge state, aim state, inventory, arrow ownership, shield state, team combat state, or timing counters yet.

`src/fight/scene.cpp:77` currently applies basic horizontal acceleration, gravity, jump, wall jump, tile collision, and drag. `Shoot` is temporarily used to advance to the next level at `src/fight/scene.cpp:95`, so real shooting is not implemented.

`src/fight/level.cpp:145` parses level size, foreground solids, background, background tiles, and player/team spawns. It ignores nearly all level entities, including treasure chests, jump pads, cracked walls, hazards, switches, lanterns, moving platforms, and special blocks.

`src/input/input.hpp:20` and `src/input/iDevice.hpp:59` already expose shoot, jump, toggle, cancel, axes, and a dash button count. There is no separate Alt Shoot action yet, even though the page requires it for Trigger Arrows.

`src/fight/collision.cpp:33` wraps solid lookups around the map and `src/fight/collision.cpp:201` wraps archers when fully outside the level. The level files have a `WrapMode` attribute, but the code does not yet respect per-level wrap settings.

`src/constants.hpp:8` runs the simulation at 10 ms per tick, or 100 ticks per second. The TowerFall page reports timings in 60 fps frames, so timing constants should be stored in seconds or converted carefully.

## Missing at a glance

| Area | Current status | Main missing pieces |
| --- | --- | --- |
| Basic movement | Partial | 8-way input quantization, variable jump height, tuned ground/air friction, fast-fall, crouch behavior, per-level wrap mode |
| Wall and ledge movement | Partial | wall slide, wall jump auto-move lockout, ledge cling, ledge slip, ghost platform fall-through |
| Shooting | Missing | aim state, reticle, quick draw, arrows, arrow inventory, homing, pickup, no-arrow feedback |
| Dodge and catch | Missing | 8-way dodge, cooldown, catch window, dodge stalling, slide dodge, dodge cancel, twitch catch, miracle catch |
| Combat resolution | Missing | arrow hits, stomp kills, shield breaking, team-safe stomps, player death, round win conditions |
| Inventory interactions | Missing | grouped arrow inventory, toggle ordering, arrow steal, team arrow share |
| Level entities | Mostly missing | chests, big chests, jump pads, hazards, cracked walls/platforms, switches, blocks, lantern/darkness support |
| Game modes | Mostly missing | Last Man Standing, Head Hunters, team scoring, round lifecycle, respawn/level rotation rules |
| Presentation | Partial | aim reticle, inventory HUD, arrow sprites, effects, catch/miracle feedback, dodge/slide/fast-fall/wall/ledge animations |
| Tests and tuning | Missing | deterministic mechanics tests, collision fixtures, timing tests, replay/debug tools |

## Timing conversion reference

The source page appears to use 60 fps frame timings. At this repo's 100 Hz simulation tick rate, use time as the source of truth and round intentionally.

| Mechanic | Source timing | Repo timing target |
| --- | --- | --- |
| Wall jump auto move | 12 frames, 0.200 s | 20 ticks |
| Ledge slip delay | 4 frames, 0.067 s | 7 ticks |
| Buffered jump window | 6 frames, 0.100 s | 10 ticks |
| Normal stored jump window | 6 frames, 0.100 s | 10 ticks |
| Post-dodge stored jump window | 12 frames, 0.200 s | 20 ticks |
| Twitch catch window | 3 frames, 0.050 s | 5 ticks |
| Minimum dodge duration | 20 frames, 0.333 s | 33 ticks |
| Maximum stalled dodge duration | 25 frames, 0.417 s | 42 ticks |
| Dodge cooldown | 25 frames, 0.417 s | 42 ticks |
| Minimum cancelled dodge duration | 2 frames, 0.033 s | 3 ticks |
| Minimum non-cancel dodge interval | 45 frames, 0.750 s | 75 ticks |
| Minimum perfect cancel interval | 27 frames, 0.450 s | 45 ticks |
| Frame-perfect miracle catch window | 5 frames, 0.083 s | 8 ticks |

## Work package 1: Basic archer control

Goal: make one archer feel correct before adding combat. This package should cover the basic "Move and Aim" and "Jump" mechanics from the page.

Implement:

- Add an explicit archer movement state model: grounded, airborne, wall sliding, ledge clinging, dodging, dead.
- Quantize movement and aim input to the 8 ordinal directions for mechanics that require TowerFall-style directional decisions.
- Separate movement direction, aim direction, and facing direction. Facing should update from movement and shooting rules instead of only initial spawn.
- Replace the placeholder acceleration/drag values with named, tunable constants for ground acceleration, air acceleration, ground friction, air friction, gravity, max fall speed, and jump speed.
- Implement variable jump height: holding Jump produces a higher jump than tapping Jump.
- Add jump input edge detection that can be reused later by buffered and stored jumps.
- Respect level `WrapMode` instead of wrapping every level in both axes.
- Keep the existing archer rendering working for stand, run, jump, fall, and duck animations.

Missing after this package:

- No arrows, no damage, no dodge, no inventory, no chests.
- Wall slide and ledge mechanics can be stubbed but should not be fully tuned until Work package 4.

Acceptance checks:

- A single player can run, stop, jump with tap-vs-hold height differences, fall, collide with solids, and wrap only on axes enabled by the level.
- Movement is deterministic across repeated runs from the same input buffer.
- The fight scene no longer uses Shoot to switch levels in normal gameplay.

## Work package 2: Shooting, aiming, and arrows

Goal: implement the core archery loop from "Shoot", "Alt Shoot", and "Arrow Toggle".

Implement:

- Add `ArrowType`, `Arrow`, and `ArrowInventory` data structures to fight state.
- Track arrow owner, position, velocity, active/stuck/held state, and arrow type.
- Give each archer a starting inventory appropriate for local testing.
- Implement aim hold: holding Shoot shows an 8-direction reticle and release fires the selected arrow.
- Implement quick draw: a fast Shoot press with direction fires immediately without a long aim hold.
- Make arrow speed independent from how long Shoot was held.
- Implement automatic arrow seeking as a small, bounded steering adjustment toward valid targets.
- Prevent grounded movement influence while the archer is holding Shoot, matching the page.
- Add no-arrow feedback state so the renderer can show the small X above the archer.
- Implement grouped inventory ordering. Non-standard arrow types should be inserted at the front; Toggle cycles the leftmost group to the end.
- Add Alt Shoot input and action plumbing. It should behave like Shoot but detonate all active Trigger Arrows owned by that player.
- Implement basic arrow pickup from stuck arrows or caught arrows.
- Add arrow rendering and simple debug drawing before polishing effects.

Missing after this package:

- Arrow homing can start as a small steering pass but needs combat tuning later.
- Trigger Arrow explosion damage can be a placeholder until combat resolution exists.

Acceptance checks:

- A player can hold to aim in 8 directions, release to fire, quick draw, run out of arrows, pick up arrows, and toggle grouped inventory.
- Multiple arrows in flight remain deterministic and synchronized in `fight::State`.

## Work package 3: Dodge, catch, crouch, and dodge-slide

Goal: implement the defensive/mobility basics from "Dodge and Catch", "Duck", and "Dodge-slide".

Implement:

- Use dash button count transitions from `input::get::dashCount` to detect Dodge presses.
- Implement 8-direction dodge selection from current direction input, defaulting to facing direction.
- Add dodge duration, cooldown, invulnerability/catch state, and post-dodge recovery counters.
- During dodge, arrows that hit the archer should be caught and inserted into inventory.
- Implement dodge stalling support: holding Dodge extends the catch period from the base duration to the max duration.
- During a dodge stall, stop the player and start cooldown only after the stalled dodge ends.
- Implement crouch while grounded when holding Down.
- Reduce crouching hitbox height and expose crouching state to rendering.
- Hide inventory and dim archer light while crouching when darkness support exists.
- Implement dodge-slide from crouch/down-forward plus Dodge. It should be horizontal, slightly faster/farther than regular dodge, keep the crouched hitbox, catch arrows, and pass through one-tile-high gaps.
- Preserve platform height during a dodge-slide that leaves a ledge until the slide animation ends.

Missing after this package:

- Dodge cancelling, twitch catch, and miracle catch remain for Work package 7.
- Detailed arrow-vs-shield and "tink" interactions should be deferred until combat has shields and arrow collision rules.

Acceptance checks:

- A player can dodge in all 8 directions, catch incoming arrows during the catch window, fail to dodge during cooldown, crouch under suitable arrows, and slide through one-tile spaces.

## Work package 4: Wall, ledge, fast-fall, and platform traversal

Goal: finish the remaining non-combat movement from "Other Abilities".

Implement:

- Fast-fall when airborne and holding Down, while still allowing horizontal input.
- Wall slide when airborne and holding toward a wall, with reduced fall speed.
- Wall jump from wall contact, with 20 ticks of forced up-and-away auto movement.
- Ledge detection for clinging to a platform edge while holding toward it.
- Ledge cling jump straight upward, and away+jump as a wall-jump-style release.
- Allow Dodge from ledge cling while preserving cling when appropriate.
- Release ledge cling when shooting.
- Ledge slip when grounded within 3 pixels of a ledge and holding Down, after a 7 tick delay.
- Ghost platform fall-through on Down+Jump, including same-tick landing fall-through without losing momentum.
- Parse and simulate Jump Pads, including their interaction with later stored jumps.

Missing after this package:

- Advanced cancel movement can now be layered on top but should not be mixed into this package.

Acceptance checks:

- A movement test map can demonstrate fast-fall, wall slide, repeated wall jumps, ledge cling, ledge slip, ghost platform fall-through, and jump pads.

## Work package 5: Combat resolution and round lifecycle

Goal: make the game objective from the page playable: defeat opponents in archery combat.

Implement:

- Arrow-vs-archer hit detection and death.
- Stomping from above as a kill condition.
- Shield state and shield breaking by stomps.
- Team-safe stomps for team modes.
- Arrow-vs-solid collision, arrow sticking, and arrow pickup.
- Arrow-vs-arrow or arrow-vs-defense "tink" behavior needed by double-tap and miracle counterplay.
- Player death state, corpse/effect placeholder, and alive filtering in the update loop.
- Round end detection when a winner or winning team remains.
- Round restart flow and level rotation.
- Basic Last Man Standing scoring.
- Head Hunters and team scoring only after Last Man Standing works.

Missing after this package:

- Treasure, powerups, variants, and full mode polish can remain disabled.

Acceptance checks:

- Two players can kill each other with arrows and stomps, rounds end, a winner is recorded, and the next round starts without corrupting deterministic state.

## Work package 6: Inventory interactions, chests, and level entities

Goal: connect TowerFall's arena objects to the combat loop.

Implement:

- Arrow steal: if a player with no arrows bumps an opponent with arrows, steal the first inventory arrow.
- Team arrow share: in Team Deathmatch and co-op-style rules, colliding teammates equalize by passing the last inventory arrow when one teammate has at least two more arrows.
- Parse and spawn TreasureChest and BigTreasureChest entities.
- Implement chest timers, chest opening, and at least normal arrow treasure.
- Add a data-driven entity layer in `Level` instead of hardcoding only spawn parsing.
- Parse and implement stage objects in priority order: JumpPad, HotCoals, CrackedWall, CrackedPlatform, Ice, MovingPlatform, switch blocks, proximity blocks, MoonGlassBlock, SpikeBall, lantern/darkness helpers.
- Keep unsupported entities visible in debug output so missing map behavior is obvious.

Missing after this package:

- Full powerup variety and variants are outside the linked page, but the entity system should be ready for them.

Acceptance checks:

- Existing `.oel` files load their combat-relevant entities into state, chests can supply arrows, and hazards/platform objects affect players and arrows deterministically.

## Work package 7: Advanced movement and catch techniques

Goal: layer in the advanced techniques from the page after basics are stable.

Implement:

- Buffered jump: accept Jump up to 10 ticks before touching a jumpable surface.
- Stored jump: accept Jump up to 10 ticks after leaving a surface, or 20 ticks after leaving during a dodge.
- Twitch catch: allow a Dodge input within 5 ticks after an arrow hit to convert the hit into a catch.
- Dodge cancelling:
  - Tap-cancel by pressing the same or another Dodge button after the dodge has begun.
  - Jump-cancel when alongside a surface or when a stored jump is available.
  - Cancelled dodge should lose catch ability immediately and enter cooldown earlier.
- Preserve tap-cancel momentum, with same-frame double Dodge not counting as a cancel.
- Super Jump: dodge-slide immediately followed by jump-cancel.
- Tap-cancelled forward dodge and forward dodge jump.
- Skating: tap-cancelled dodge-slide on ground.
- Hyper Jump: dodge-slide, immediate tap-cancel, then jump.
- Tap-cancelled upward dodge, downward dodge, Hyper Wing, Elevator Jump, and Wall Jump Cancel.
- Miracle Grab: cancel immediately after catching an arrow, emit a special feedback event, and preserve the appropriate momentum/cooldown behavior.
- Double tapping: support two valid shoot inputs or an equivalent input model that can fire two arrows in near succession.

Acceptance checks:

- Each advanced technique has a small deterministic test or replay fixture.
- Cancels change vulnerability exactly when the cancel occurs.
- Miracle catches can be distinguished from normal catches in state/events for sound and debugging.

## Work package 8: Rendering, HUD, audio hooks, and feedback

Goal: make implemented mechanics readable to players.

Implement:

- Aim reticle for 8 directions.
- Inventory display above each archer, hidden when empty or crouching in darkness.
- No-arrow X feedback.
- Arrow sprites, stuck arrows, trigger arrow states, explosion placeholder.
- Crouch, dodge, dodge-slide/roll, fast-fall, wall-slide, ledge-cling, hit/death, and catch animations.
- Shield, chest, jump pad, hazard, and special entity rendering.
- Sound/event hooks for shoot, catch, twitch catch, miracle catch, dodge, death, chest open, and trigger detonation.
- Debug overlays for hitboxes, contact normals, current movement state, dodge timers, jump buffer windows, and inventory.

Acceptance checks:

- The player can tell why they died, whether a dodge can catch, what arrow will fire next, and whether a special movement state is active.

## Work package 9: Tests, tuning, and rollback safety

Goal: make the mechanics reliable enough for a synchronized fight scene.

Implement:

- Unit tests for inventory ordering, arrow toggle, arrow steal/share, timing windows, dodge cooldowns, and wrap mode.
- Collision fixtures for ground, wall, ceiling, ledge, one-tile tunnel, ghost platform, and moving platform cases.
- Deterministic replay tests for each work package.
- Fixed-step state serialization coverage for arrows, chests, entity state, timers, effects, and mode state.
- Gameplay tuning config or constants grouped by mechanic so TowerFall-like values can be adjusted without hunting through the update loop.
- A debug input/replay harness to reproduce advanced movement sequences.

Acceptance checks:

- Running the same replay twice produces identical fight state.
- Timing-sensitive features use named constants and have tests covering early, valid, and late inputs.

## Suggested implementation order

1. Finish Work package 1 until single-player movement is stable.
2. Add Work package 2 so the core verb, shooting arrows, exists.
3. Add Work package 3 so arrows can be defended against and movement starts feeling like TowerFall.
4. Add Work package 5 before broad entity work, because combat resolution will define what chests and hazards need to affect.
5. Add Work package 4 and Work package 6 in parallel only if ownership is split cleanly between archer movement and level entities.
6. Add Work package 7 only after basic movement, dodge, arrows, and collision have deterministic tests.
7. Keep Work package 8 and Work package 9 active throughout rather than leaving all feedback and tests to the end.

## First concrete coding tasks

1. Replace the temporary Shoot-to-next-level behavior with a debug-only command or remove it from normal fight updates.
2. Expand `fight::Archer` with explicit movement, aim, jump, and dodge timing fields.
3. Add a mechanics constants header for movement timings and convert source-page frame values to repo ticks.
4. Teach `Level` to store wrap mode from the level XML.
5. Add deterministic tests for ground jump, variable jump hold, wall jump, and wrap mode before adding arrows.
