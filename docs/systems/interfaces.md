# Screens within a screen: HUD, map, lockers and terminals

You've got an alien after you, the oxygen is ticking down, and suddenly a panel unfolds into a full-screen little machine with its own hands, cards and blinking messages. That's not a random UI bolted onto the FPS: those interfaces are a significant part of how *In Extremis* tells you what is happening.

## The original 320×200 canvas

The classic presentation is **320×200**. The first-person viewport takes the upper **320×150**, leaving a **50-pixel HUD** underneath. The port keeps that basic layout and presents it in a 4:3-style view rather than turning the game into a permanently stretched widescreen image.

The lower panel carries the essentials: health/energy, oxygen, equipped weapon and ammunition, body/status graphics, messages, and the motion detector when powered.

The HUD reads **real simulation state**. It shouldn't silently invent a resource level because a graphic looks better full. The corresponding code is in [`original_hud.cpp`](../../src/extremis/ui/original_hud.cpp) and [`survival_session.cpp`](../../src/extremis/gameplay/survival_session.cpp).

## The map isn't free

One of the first mysteries to solve was why the map wasn't available at the start. The answer lives in the game: the early terminal is a **map transfer station**, not a supply locker.

In level 1, the identified panel is at **sector 1, wall 6**, a `0x3606` type surface at world segment `(3200,1000) → (3200,1200)`. The map permission is awarded through that interface. After transfer, the map still has a safety rule: **if a nonremoved enemy belongs to a connected sector, it won't open**, showing `ATTENTION: AN ALIEN IS NEAR!!`.

The map's block condition uses the original sector-connection traversal, not a line-of-sight ray. So that alien can be behind you and still ruin your cartographic plans.

[`original_map.cpp`](../../src/extremis/ui/original_map.cpp) renders the original maps and marker. In the port, **M** is a convenient map key and the physical original map scancode is also accepted. The developer-only TAB menu is a separate feature in Debug builds.

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

The presentation logic is spread across [`original_interactions.cpp`](../../src/extremis/ui/original_interactions.cpp), [`interaction_animation.cpp`](../../src/extremis/ui/interaction_animation.cpp) and the stateful [`item_session.cpp`](../../src/app/session/item_session.cpp). They share resources, but they must not accidentally duplicate the inventory or door state.

`ItemSession` advances its panel phases in [`item_animation.cpp`](../../src/app/session/item_animation.cpp).
The [campaign terminal audit](../reference/exe/code-terminals.md) identifies every stock panel, card requirement and displayed zone code.

The SDL event routing for those panels lives in [`item_input.cpp`](../../src/app/session/item_input.cpp).

**Port adaptation.** Options → Gameplay → Ignore locker codes opens supply and weapon lockers directly. Map terminals still require the zone code and card readers still check their cards. Turning the option off restores the zone's earned authorization status. The title and pause menus save the change immediately as `ignore_locker_codes` in `settings.ini`. The default for new installations and older settings files is off.

**Port-tested, 2026-11-24.** The setting reached `ItemSession::open`, but the opening animation drew the keypad and its code prompt during `PanelSlide` even when the locker had skipped code entry. The bypass now starts with the locker door sliding into view. The renderer slides a keypad out only after a keypad acceptance. On the original C200 supply locker (sector 8, wall 8, LAB offset `0xe38`) and weapon locker (sector 8, wall 6, `0xe28`), the regression compares every opening frame with a second rendering in which only the keypad sprite is marked. Both reach the contents prompt without code input or pickup. Switching the option off in the pause menu brings back the interactive keypad on the next visit. Switching it on again opens the same locker directly. The Debug gameplay loop also reproduced all three visits at the supply locker.

Run `interaction_tests --locker <test-directory> <path/to/GAME.LAB>` to repeat with the reference LAB and its adjacent `DATA` directory. CTest runs the same checks with fixture resources. [Reference LAB hash and device inventory](../reference/exe/code-terminals.md)

### Pickup movie presentation

When a locker closes after a successful pickup, `ItemSession` runs `MovieIn`, `Movie` and `MovieOut`. All three phases of `A_ARME.ANM` replace the entire logical 320×200 DOS frame, including the 50 HUD rows. The first-person presenter uploads that frame at its logical size and fits it to a centred 4:3 rectangle in the current SDL drawable. World rendering resumes at the selected aspect and internal scale when the movie ends.

**Port-tested, 2026-11-19.** The pre-refactor revision `48d7f48` already sent the opaque movie through the world/HUD compositor. The panoramic path introduced in `9cb4c25` retained the world at the sides, remapped the movie's corner pixels and extended the HUD panel behind it. The higher-resolution compositor also used colour equality to decide which pixels belonged to a transparent HUD overlay. The original-resource probe in `hud_tests DATA` compares the final SDL pixels against the standalone 4:3 presentation across all four aspect settings, scales 1×–6× and MAX, including drawable changes during playback. The local `A_ARME.ANM` had SHA-256 `93a8793214db7dbf4eac449cc530042e1da3b83e78a9fa3bef14c752800b59a8`. This result covers SDL software rendering.

## One button, several meanings

The contextual **E** action handles nearby panels and doors as appropriate; inside a modal it confirms. **I** opens and closes inventory. On the zone keypad, Escape and Backspace cancel immediately. As a port adaptation, pressing Down from any key in the bottom row selects EXIT (SALIR in Spanish, QUITTER in French). E starts the panel closing animation and discards the input. Escape opens the port menu on the other item screens. The input routing is designed so a key that opened the panel doesn't immediately confirm whatever happens to be selected.

**Port-tested on macOS, 2026-11-24.** The input regression navigates to SALIR, returns to the letter grid, cancels with each control and reopens the keypad with empty input. The original-resource SDL software capture checks ink for each of the five Spanish glyphs in the final selected button at 640×480, 960×540 and 1260×540, across all four aspect choices and scales 1×–6× plus MAX. All 84 configurations passed in Debug and Release.

With a charged battery, **E** on the battery slot opens its mode selector.
Navigate with **W/S** or **↑/↓** and confirm with **E** to return to play.
Battery cabinets also use the current modal controls to confirm an exchange
when the installed battery still has charge. Both panels pause the
gameplay battery clock. [Battery, detector and optics](items.md#battery-detector-and-optics).

This was an actual reconstruction pitfall: a user could open inventory but not close it, or see the wrong card set because the reader filtered out incompatible cards before displaying them. The current model separates *what cards you have* from *whether a particular card is accepted*.

### A small piece of original character

The elevator cursor, Yes/No masks and the little hand animation have their own anchors and visual conventions. These are not superficial details; they determine whether the player can understand what E will actually do. The reconstructed modal states keep original image anchors instead of drawing everything at guessed top-left positions.

## What isn't fully restored?

The exact timing and palette transitions of every modal aren't finished. A promising observation from the original game is that the scene behind some locker/elevator modals becomes **black and white**. That effect still needs verified scoping: whole screen, background only, or palette change. We should prove it before implementing it.

Likewise, the audio heard during map transfer and map open/close needs separate verification. Some late-level interactions and the ending remain under test.

**Original references:** early map panel `0x3606` data, interaction dispatch near `0xa608`, animation routines listed in source comments, connection traversal `0x114ba–0x1185c`. For the sector model, start with [Levels](levels.md).

Player death temporarily owns the full 320×200 canvas and consumes interface
input. After GAMEOVER or the victory ending, the port opens its custom
main menu without RESUME. The original GAMEOVER text is documented in
[Death sequences](../reference/exe/player-death.md).

Choose **LOAD GAME** in the custom menu and type the eight-character
continuation code. Enter confirms and Backspace erases the last character.
An accepted code starts a fresh party with the checkpoint's preset inventory
and vitals. Eligible elevator and shuttle arrivals can display a quick code
while the world is paused. Each notice has one use per program run, including
when its code was already entered. The code itself stays reusable. The six
characters requested at terminals belong to a different system.
[Password continuation](../reference/exe/password-continuation.md).
