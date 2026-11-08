# Inventory: no infinite backpack here

The original *In Extremis* inventory is wonderfully unforgiving. You don't carry twelve guns, forty healing items and a mountain of spare tanks. You have **specific slots**. A container may remain occupied even when it's empty. And picking up a supply can mean **exchanging** what you're carrying, not just collecting more.

## What the player actually carries

The reconstructed [`Inventory`](../../include/player_inventory.h) uses fixed storage rather than an endlessly growing list:

| Slot group | Capacity | Original-style behaviour |
| --- | ---: | --- |
| Cards | **3** | Selection and replacement; not just an unordered keyring |
| Injectors | **2** | Restore health when used |
| Oxygen reserve | **1** | Exchange oxygen values with the main tank |
| Mine | **1** | Stored as an inventory item; use mechanics still being investigated |
| Battery | **1** | Supplies the detector or optical equipment system |
| Equipped weapon | **1** | Current weapon plus ammunition |

These are the port's explicit fields; some original replacement rules remain incomplete in the public build. [`player_inventory.cpp`](../../src/player_inventory.cpp) owns transfer and use operations, and [`item_session.cpp`](../../src/item_session.cpp) handles the interaction with the original panel screens.

## A bottle that swaps instead of filling

Oxygen is a good example of why not to guess. Using a reserve doesn't blindly top up the main tank:

```text
before: main oxygen = 10000; reserve = 50000
after:  main oxygen = 50000; reserve = 10000
```

The underlying operation exchanges values. Think of swapping tanks, not drinking a health potion. An injector, by contrast, restores health to full.

## Reading a panel is not taking its contents

The DOS separates *opening the interface*, *choosing an item*, *confirming the transfer* and *updating inventory*. The port mirrors that split so that pressing the action key on a locker doesn't automatically vacuum up its contents.

A panel may offer a code entry, supply, weapon, card reader, map transfer, code enquiry or elevator interface. The underlying world object and the modal presentation must stay in sync — a pretty animation is no use if the item disappears twice.

### An early security example

In level 1, the **C001** card can be used at the security reader associated with **door 7**. The card remains in inventory; the reader unlocks the access, and the player still has to open the door normally afterwards. Presenting a wrong card should display the original rejection rather than quietly deleting it from the menu.

The original early map terminal is **not** a seemingly empty locker. It transfers map permission. That distinction is documented in [HUD, map and terminals](interfaces.md).

## Codes, zone tables and other peculiarities

Zone access codes and their target lists are part of the level data. The loader reads them from header `+0x0E` in `GAME.LAB`. The project has also recovered original elevator destinations and locker contents across levels, but the full campaign's interaction chain still needs end-to-end playtesting.

**Do not confuse:**

- A **zone code** used in an interface.
- A **physical card** stored in an inventory slot.
- A **reader state** that has already accepted a card.
- A **door connection-state word** that determines whether a passage is currently traversable.

Those are separate pieces of state. They can affect one another without becoming the same variable.

## Battery, detector and the optics question

The original new-game template begins with the battery slot absent (`-1`). A working battery can be assigned to the motion detector; the original interface also contains an optical-device choice. The original control logic found so far switches between these uses rather than enabling both simultaneously through that menu.

The detector shows up to ten enemy contacts in its own HUD area. It doesn't require direct line of sight: a target can be behind a wall and still register. See [Enemies](enemies.md) and [Interfaces](interfaces.md).

## What still needs work?

Inventory-full replacement cases, mine placement and explosion, some special reader sequences and a few endgame interactions are still under investigation. The existence of a mine item or a possible alien nest is **not proof** that every guessed explosion rule is correct.

**Where to look:** [`player_inventory.h`](../../include/player_inventory.h), [`level_items.cpp`](../../src/level_items.cpp), [`item_session.cpp`](../../src/item_session.cpp), [`original_interactions.cpp`](../../src/original_interactions.cpp). Original case studies include the C001/door-7 flow and the map-transfer panel; they are not templates for every panel in the game.
