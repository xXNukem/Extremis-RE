# Inventory: no infinite backpack here

The original *In Extremis* inventory is wonderfully unforgiving. You don't carry twelve guns, forty healing items and a mountain of spare tanks. You have **specific slots**. A container may remain occupied even when it's empty. And picking up a supply can mean **exchanging** what you're carrying, not just collecting more.

## What the player actually carries

The reconstructed [`Inventory`](../../include/extremis/gameplay/player/player_inventory.h) uses fixed storage rather than an endlessly growing list:

| Slot group | Capacity | Original-style behaviour |
| --- | ---: | --- |
| Cards | **3** | Selection and replacement; not just an unordered keyring |
| Injectors | **2** | Restore health when used |
| Oxygen reserve | **1** | Exchange oxygen values with the main tank |
| Mine | **1** | Inventory bomb. DOS places it at the player’s current local position on confirmation |
| Battery | **1** | Supplies the detector or optical equipment system |
| Equipped weapon | **1** | Current weapon plus ammunition |

These are the port's explicit fields; some original replacement rules remain incomplete in the public build. [`player_inventory.cpp`](../../src/extremis/gameplay/player/player_inventory.cpp) owns transfer and use operations, and [`item_session.cpp`](../../src/app/session/item_session.cpp) handles the interaction with the original panel screens.

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

## Battery, detector and optics

NEW GAME starts without a battery. The first stock battery is in level 2, sector 13 (zero-based). With a charged battery, open inventory with **I**, select **Battery** and press **E**. Use **W/S** or **↑/↓** to select a mode, turn the current one off, or leave it unchanged. **E** confirms and returns to play. Both modes consume the same charge at the same rate and shut off at zero. Either mode lights the battery indicator, and radar is the one that displays contacts.

Optics changes the gameplay image and HUD to green by doubling the green palette channel up to the VGA limit. A palette entry with zero green appears black. Optics suppresses colored lamp tint. Power Surging can darken the green image while the battery still has charge.

Open a battery cabinet and confirm the pickup. When the installed battery
still has charge, choose **Take battery** or **Keep battery** and press **E**.
Taking it puts the old battery in the cabinet with its charge rounded down
to the cabinet's storage unit. An empty installed battery leaves zero. The
cabinet empties when the player had no battery. [Optic system, charge rules and cabinet locations](../reference/exe/colored-lights.md#optic-system-and-battery).

The detector shows up to ten enemy contacts in its own HUD area. It doesn't require direct line of sight: a target can be behind a wall and still register. See [Enemies](enemies.md) and [Interfaces](interfaces.md).

## Inventory bomb

DOS consumes the bomb slot when the inventory choice is accepted. After the arming animation the fuse advances in gameplay and warns of five seconds. The charge flashes and shakes the view at detonation. A blast within 207 horizontal map units on each axis disables an eligible generator and leaves its broken sprite in place. DOS stores that variant in the working LAB when travelling. An armed fuse follows the saved sector index and local coordinates into the destination level. The port implements placement, damage and generator persistence. The DOS arming animation and live timing comparison remain pending. [Bomb timing, damage and generator data](../reference/exe/nests-and-bomb.md).

Inventory-full replacement cases, some reader sequences and other endgame interactions are still under investigation.

**Where to look:** [`player_inventory.h`](../../include/extremis/gameplay/player/player_inventory.h), [`level_items.cpp`](../../src/extremis/gameplay/items/level_items.cpp), [`item_session.cpp`](../../src/app/session/item_session.cpp), [`original_interactions.cpp`](../../src/extremis/ui/original_interactions.cpp). Original case studies include the C001/door-7 flow and the map-transfer panel; they are not templates for every panel in the game.
