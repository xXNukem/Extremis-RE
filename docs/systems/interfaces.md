# Screens within a screen: HUD, map, lockers and terminals

You've got an alien after you, the oxygen is ticking down, and suddenly a panel unfolds into a full-screen little machine with its own hands, cards and blinking messages. That's not a random UI bolted onto the FPS: those interfaces are a significant part of how *In Extremis* tells you what is happening.

## The original 320×200 canvas

The classic presentation is **320×200**. The first-person viewport takes the upper **320×150**, leaving a **50-pixel HUD** underneath. The port keeps that basic layout and presents it in a 4:3-style view rather than turning the game into a permanently stretched widescreen image.

The lower panel carries the essentials: health/energy, oxygen, equipped weapon and ammunition, body/status graphics, messages, and the motion detector when powered.

The HUD reads **real simulation state**. It shouldn't silently invent a resource level because a graphic looks better full. The corresponding code is in [`original_hud.cpp`](../../src/original_hud.cpp) and [`survival_session.cpp`](../../src/survival_session.cpp).

## The map isn't free

One of the first mysteries to solve was why the map wasn't available at the start. The answer lives in the game: the early terminal is a **map transfer station**, not a supply locker.

In level 1, the identified panel is at **sector 1, wall 6**, a `0x3606` type surface at world segment `(3200,1000) → (3200,1200)`. The map permission is awarded through that interface. After transfer, the map still has a safety rule: **if a nonremoved enemy belongs to a connected sector, it won't open**, showing `ATTENTION: AN ALIEN IS NEAR!!`.

The map's block condition uses the original sector-connection traversal, not a line-of-sight ray. So that alien can be behind you and still ruin your cartographic plans.

[`original_map.cpp`](../../src/original_map.cpp) renders the original maps and marker. In the port, **M** is a convenient map key and the physical original map scancode is also accepted. The developer-only TAB menu is a separate feature in Debug builds.

## Modals have their own world rules

When a locker, reader, elevator or inventory screen is active, **world simulation is suspended**. Movement, firing and accidental use actions must not leak through to the scene. Releasing the modal must not replay keyboard or mouse movement that happened during it.

The original resources supply a range of surprisingly elaborate sequences: a panel grows into place, a hand enters or withdraws, a code is checked, shutters open, cards are inserted, and a pickup movie can play. The port models those phases explicitly rather than redrawing a static fake menu.

| Interface | What it asks the player to do |
| --- | --- |
| Code panel | Enter/check a zone or equipment code |
| Supply / weapon locker | Open, inspect, accept/reject or exchange contents |
| Card reader | Present a stored card; wait for acceptance/rejection |
| Map-transfer terminal | Grant map access after the original sequence |
| Code enquiry | Reveal/use code information rather than dispense an item |
| Elevator | Choose an available destination and travel |
| Inventory | Inspect fixed slots and use/reorganize carried items |

The presentation logic is spread across [`original_interactions.cpp`](../../src/original_interactions.cpp), [`interaction_animation.cpp`](../../src/interaction_animation.cpp) and the stateful [`item_session.cpp`](../../src/item_session.cpp). They share resources, but they must not accidentally duplicate the inventory or door state.

## One button, several meanings

The contextual **E** action handles nearby panels and doors as appropriate; inside a modal it confirms. **I** opens and closes inventory; Escape cancels a modal. The input routing is designed so a key that opened the panel doesn't immediately confirm whatever happens to be selected.

This was an actual reconstruction pitfall: a user could open inventory but not close it, or see the wrong card set because the reader filtered out incompatible cards before displaying them. The current model separates *what cards you have* from *whether a particular card is accepted*.

### A small piece of original character

The elevator cursor, Yes/No masks and the little hand animation have their own anchors and visual conventions. These are not superficial details; they determine whether the player can understand what E will actually do. The reconstructed modal states keep original image anchors instead of drawing everything at guessed top-left positions.

## What isn't fully restored?

The exact timing and palette transitions of every modal aren't finished. A promising observation from the original game is that the scene behind some locker/elevator modals becomes **black and white**. That effect still needs verified scoping: whole screen, background only, or palette change. We should prove it before implementing it.

Likewise, the audio heard during map transfer and map open/close needs separate verification. Some late-level interactions and the ending remain under test.

**Original references:** early map panel `0x3606` data, interaction dispatch near `0xa608`, animation routines listed in source comments, connection traversal `0x114ba–0x1185c`. For the sector model, start with [Levels](levels.md).
