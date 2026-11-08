# OBJET: the scenery is doing things behind your back

Those things dripping from the ceiling? At first they look like broken sprite positions. Then you look at the DOS code and discover they're **supposed to fall, reset and fall again**. *In Extremis* does a surprising amount of its environmental animation without conventional sprite sheets.

The game has **ten OBJET banks**, selected per level. Some images are placed in the world; others are source material for patches copied onto other images. The same bank can contain static decoration, vertical movement, tiny animated components and objects that block movement.

## What an object record actually says

A level's `EntityA` entry contains four 16-bit words:

```text
vertex | type | Param1 | Param2
```

The vertex locates the object in the sector. The type contains the image index and flags. **`Param1` controls its vertical position**: relative to the wall projection, `0` is the floor and `256` is the ceiling. **`Param2` can be a timer**, but it doesn't mean the same thing for every object.

Physical height comes from an original **per-bank, per-image table**. It controls the sprite's size, not where it's placed. Image anchors come from the SPR data and can change between frames. Vertical flip `0x0100` transforms the anchor; it doesn't arbitrarily move floor objects to the ceiling.

This distinction matters. No static object has been *proven* misplaced solely by comparing those recovered values. To diagnose an apparent misplaced prop, we need its **level, bank, image index, type and Param1**, not just a screenshot that looks odd.

## Three animation tricks

| Mechanism | What changes? | Why it matters |
| --- | --- | --- |
| **Whole-image selection** | The current image index | Standard frame changes, but driven by DOS counter rules |
| **Pixel composition** | Parts of the bank image in memory | Patches overwrite rectangles, including transparent index 0 |
| **Vertical displacement** | The instance's `Param1` / world Z | Falling objects can keep the same image throughout |

The distinction is deliberate. A pixel patch may modify one image shared by **every instance** of that bank. A falling object instead changes **one instance's world position**. Updating the first once per *instance* would accidentally speed the animation up.

### Whole-image animation catalogue

All indices below are **zero-based**. The following sequences have been traced from the saved executable disassembly:

| Bank | Images | Cycle / special rule |
| --- | --- | --- |
| OBJET2 | `9–11` | `9 → 10 → 11 → 9` on even global-counter steps |
| OBJET3 | `15–18` | `15,15,16,16,17,17,18,18,18,18,17,17,16,16,15,15` |
| OBJET3 | `22–28` | 32-step cycle: `22` for 0–16, `23..28..23` for 17–27, then `22` for 28–31 |
| OBJET4 | `26–29` | `26,27,28,29,29,28,27,26` |
| OBJET9 | `13–16` | `13,14,15,16,16,15,14,13` |
| OBJET9 | `19–22` | `19,20,21,22,22,21,20,19` |
| OBJET9 | `23–26` | `23,24,25,26,26,25,24,23` |
| OBJET6 | `13–22` | **Individual Param2 timer**, not the shared frame counter |

The OBJET4 and OBJET9 sequences use `tick & 7`. Other sequences have pauses and repeat endpoints. These are **exact phase patterns**, not proof of an exact speed in seconds: the port's shared 18 Hz animation step is a practical adaptation pending DOS cadence calibration.

### The eleven falling objects

These are world-position animations, **not** mislabelled frame sequences:

| Family | Bank / image pairs | Physical height |
| --- | --- | ---: |
| A | OBJET1[14], OBJET2[3], OBJET4[11], OBJET6[2], OBJET7[2], OBJET8[14] | 10 |
| B | OBJET3[5], OBJET4[15], OBJET6[5], OBJET7[23], OBJET8[12] | 20 |

The original update subtracts **20** from `Param1`, then adds **256** when it goes negative. The fall loops. It's easy to mistake that reset for a sprite teleporting to the wrong height — but it's part of the effect.

An early port implementation animated *every* image 14 regardless of bank. The corrected selection has been implemented and tested in the **local working tree**; the public `main` snapshot may not yet contain it.

### Patches and rotating columns

The original draws animations directly into mutable bank images. Think of an old flipboard: most of the picture stays put while a strip or small rectangle changes.

| Bank | Animated patch destination images | Vertical pixel rotation targets |
| --- | --- | --- |
| OBJET1 | 15–17 | 4, 5 |
| OBJET2 | 18–20 | — |
| OBJET3 | 13, 19 | 14 |
| OBJET4 | 21 | — |
| OBJET5 | 12, 13 | 11 |
| OBJET6 | — | — |
| OBJET7 | 25, 26 | — |
| OBJET8 | 20, 21 | 22 |
| OBJET9 | 17, 18 | — |
| OBJET10 | — | — |

The patch source images live in the same bank, often **after the range of normal placed-object images**. Source index zero pixels are copied too — they're not filtered away as transparent in this operation. Column rotation is **cumulative**, applied once per relevant tick, and alternates processed columns by counter parity. Skipping intermediate ticks or composing once per visible object produces different pixels.

[`src/sprite_scene.cpp`](../../src/sprite_scene.cpp) contains the original patch and rotation descriptors. The active local development work connects that compositor to a shared 16-bit step counter; see [Known differences](../reference/known-differences.md) before assuming those local changes are already in `main`.

## OBJET6 and the clock that isn't 18 Hz

OBJET6[13–22] is special. Each object owns a **16-bit `Param2`**. On an eligible processing pass the DOS subtracts a delta read from `[0x1428]`. Only a **negative signed 16-bit result** advances a frame, by one image at most. Zero does **not** advance. The negative result is retained between images; only the wrap **22 → 13** resets `Param2` to **800**.

Where does that delta come from? The original PIT interrupt runs at roughly **160 ticks/second** (divisor 7457). A counter accumulates since the last main-loop reset; the loop resets it to **1 after processing sectors and objects**. Modal suspension can stop increments. So `800` is nominally five seconds of PIT units, **not** a guaranteed five-second frame or cycle.

Another catch: DOS only visits instances from sectors processed in that render/world pass, excludes certain `0x2000` objects and can reject a transformed vertex marker of `0xffff`. The port's ray-based renderer does not emit that exact DOS vertex-visibility information. An implementation that updates every OBJET6 instance at 18 Hz would therefore be an **explicit approximation**, not exact reconstruction. This animation remains an open integration task.

## Solid or walk-through? The flags decide

The original collision routine (`0x10db7–0x10ff6`) does **not** inspect sprite pixels or vertical height. It works in XY, per-sector, with the following rules:

- **`type & 0x2000`**: exclude this object.
- **`type & 0x1000`**: physical blocking is enabled, *if* the object has a positive collision extent.
- The half-extent `r` comes from the **bank-and-image table**, not the image dimensions.
- The rectangle is half-open: **`[x-r, x+r)` × `[y-r, y+r)`**.
- Position is corrected against its previous position. Enemies also turn by **±80 original angular units** when the collision response applies.

For example, an OBJET1 image 2 lamp can have radius 200 yet remain walk-through if its instance type is `0x4002` instead of a blocking `0x5xxx` form. **A size in the table does not automatically mean it's solid.** The eleven falling sprites all have zero half-extent.

### Original collision half-extents

The next table contains recovered **16-bit XY half-extents**, in image-index order starting at **0**. Use the instance flags to decide whether the value actually blocks.

| Bank | Half-extents, index 0 onward |
| --- | --- |
| OBJET1 | `200,200,200,200,80,80,80,0,0,80,80,80,0,0,0,80,80,80,80,80,80,150` |
| OBJET2 | `200,200,0,0,100,80,80,80,100,100,100,100,80,0,80,150,0,80,80,100,0` |
| OBJET3 | `200,200,150,0,0,0,0,80,80,100,100,80,0,150,80,80,80,80,80,80,100,100,0,0,0,0,0,0,0` |
| OBJET4 | `200,200,80,80,100,0,80,80,0,100,80,0,0,0,0,0,0,80,80,80,100,100,0,0,0,0,80,80,80,80` |
| OBJET5 | `200,200,100,100,100,80,80,80,150,80,100,80,100,100` |
| OBJET6 | `200,200,0,0,0,0,0,80,80,80,80,0,0,0,0,0,0,0,0,0,0,0,0,100,150,100,80,80,80,100,100` |
| OBJET7 | `200,200,0,0,0,150,80,80,80,0,0,100,80,80,100,80,0,150,100,80,80,100,0,0,0,80,80` |
| OBJET8 | `200,200,100,80,80,80,80,80,0,100,80,0,0,0,0,0,0,80,100,100,80,100,80` |
| OBJET9 | `80,80,80,80,80,100,100,100,80,100,100,120,100,100,100,100,100,80,80,100,100,100,100,100,100,100,100` |
| OBJET10 | `120,100,100,80,100,80,100,80,100,100,100,120,100,100,80,120,100,80,120` |

The data came from the original executable tables; it doesn't authorize guessing collision boxes from the drawing. None of the identified whole-image cycles changes its half-extent, which makes those animations easier to keep consistent.

## Reconstruction status and places to check

The published core includes [`sprite_scene.cpp`](../../src/sprite_scene.cpp), [`sprite_bank.cpp`](../../src/sprite_bank.cpp) and [`sprite_renderer.cpp`](../../src/sprite_renderer.cpp). The newest work on step-driven animation and the player's OBJET collision is **still local and awaiting its next code commit**. Enemy-object collision, its angular correction, OBJET6 visibility eligibility and exact animation cadence remain open.

**Original references:** OBJET bank dispatch `0x7d00–0x7db4`, height table reads near `0x11f83`, vertical projection `0x11ff2–0x12007`, patch/column routines around `0x14d1f–0x1517e`, collision `0x10db7–0x11028`, individual OBJET6 update `0x11e79–0x11ea5`.

If a prop looks wrong, report **level + sector + OBJET bank + image index + flags + Param1** when possible. That turns “the ceiling is haunted” into a reproducible test.
