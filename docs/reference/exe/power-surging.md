# Power surge

Inspected 2026-11-20 with [the bounded LLVM disassembler](../../../tools/inspect_original_code.py) and an immediate-reference scan of `EXTREMIS.EXE`, SHA-256 `7b2883d5fccdd75d75e4832cf55a82a46358ffb25d7c037b2fa12551ba32f397`. Offsets are absolute file positions. Brackets indicate runtime addresses.

## EXE-confirmed event and timing

The exact null-terminated English message starts at `0x21a90` (runtime `0x21890`): `ATTENTION\r\n POWER IS SURGING!!!`. The only immediate reference to that runtime pointer is at `0x16bf1`. It is passed to the common text queue `0x14399`, which deduplicates by pointer and keeps up to five messages, dropping the oldest on insertion at capacity. CR begins the next line and LF is skipped by the renderer (entry `0x14415`). Repeated queue requests therefore do not restart its typewriter progress. At `0x1454d–0x14556` the text renderer adds the shared delta to the entry age. At `0x14574–0x14596` it removes entries from the front whose signed age is at least 800, so the warning can persist after the darkening stops.

`0x6aea` initializes the signed dword `[0x608f]` to 9600 during campaign setup, before the initial level load at `0x6b97`. The only other direct references to this address are the gameplay read and final write below. The main loop `0x7039` calls the player update at `0x1656a`, including the surge block `0x16be0–0x16c56`. Neither the block nor its call checks level, sector, palette bank, security progress or proximity. This rule applies across the 30 playable levels on ordinary player updates, with the same counter carried between levels. Blocking interfaces do not independently run this block. See [the independent security countdown](self-destruction.md) for the relationship to the security countdown.

```text
remaining = signed32([0x608f]) - unsigned16([0x1428])
if remaining < 0:
    queue(message at runtime 0x21890)
    request sound bank slot 14
    if [0x53f9] == 0:
        [0x53f9] = -1
        [0x53fb] = (rng() & 63) + 30
        [0x53fd..0x53ff] = (0, 0, 0)
    if remaining < -640:
        remaining = (rng() & 65535) + 9600
[0x608f] = remaining
```

Zero is still quiet. Exact −640 is active and does not reload. The first update past −640 still requests warning/audio and may set a darkening pulse before rearming. Quiet intervals after the first surge are 9600–75135 counter units. Active duration is governed by the strict threshold and update granularity. If an update crosses both thresholds at once, it requests the effects once and then rearms. The main loop resets the shared PIT delta to 1 at `0x71d6`. The number of requests depends on the deltas that reach the player update. These are counter units, not measured seconds. The seed depends on the rest of DOS execution. The RNG entry is `0x3dac`. The amount draw at `0x16c17` is skipped if another effect is already selected, but rearming at `0x16c43` still draws when due.

## EXE-confirmed presentation and state effects

- The transient slot `[0x53f9]` is cleared on entry to each player update at `0x16584`. An effect already present when the surge runs, including low-oxygen white starting at `0x16b22`, suppresses its black pulse. The warning and sound request still execute. Later transient writers, such as a bomb, can overwrite the slot before palette application.
- `0x74e2–0x75a4` selects the transient slot first, then eligible colored-lamp tint, then normal palette restoration. It blends the current base palette at pointer `[0x50f4]` toward black by 30–93% on each qualifying active update. `0x14611` applies the blend to all 256 VGA DAC entries. The indexed pixels themselves do not change for the flash. World, sprites, HUD and warning text displayed under this palette are darkened together. The ordinary restore path at `0x7519–0x752c` reinstalls the selected base palette after an effect unless another effect is selected. There is no fade-out counter or fixed alternating light phase.
- `0x16bff → 0x13103` requests the same original BANK[14] as self-destruction. The callee sets the audio request byte when audio is enabled. See the [bank metadata](self-destruction.md#countdown-display-alarm-and-timebase) for the sample and loop settings. The audio scheduler at `0x12f1c–0x12f68` keeps a continuous voice only while it has a request, and the request bytes are cleared for the next pass at `0x13078–0x1308c`. The surge stops requesting the alarm during its quiet interval. Another source requesting BANK[14] can keep it audible. This request and the text queue are the event's non-palette outputs. The block neither reads nor writes health, oxygen, enemies, energy, inventory or level objects. Oxygen decreases in preceding code on the same update and the security countdown is checked independently immediately afterward at `0x16c57`.

## State cross-references (2026-11-23)

**EXE-confirmed.** Rechecked the indexed EXE hash above. A little-endian address scan for the four bytes of the runtime countdown at `[0x608f..0x6092]`, followed by decoding each matching operand, found the initializer at `0x6aea`, the read at `0x16be0`, and the write at `0x16c52`. There is no direct reader in enemy AI, door/connection logic, machinery, or other equipment. The dword at `[0x6093]` is adjacent but separate. The bomb sets that dword at `0x1037e`, and the player reads it at `0x167bd` to apply shake. Similarly, detection and battery logic at `0x13e04–0x13e5c` and `0x16b5b–0x16bca` read their own flags and charge. Neither the surge timer nor the palette effect disables contacts, drains extra charge, stops doors, damages the player, or awards progress. This direct-operand scan cannot prove the absence of a computed alias. The bounded trace below identifies one real indirect path through state shared with other routines.

The text queue sits immediately before the countdown. Its five ten-byte entries occupy `[0x605d..0x608e]`. The insertion at `0x143b3–0x1440c` and removal at `0x14574–0x14596` shift only four entries, so the queue's computed writes stop before `[0x608f]`. The warning can evict an older HUD message when first queued. The sound call at `0x13103–0x13122` writes only the selected audio request byte (index 14 in this call), subject to the audio enabled flag.

**EXE-confirmed indirect effect.** Each call at `0x16c17` or `0x16c43` mutates the shared RNG words `[0x2f58/0x2f5a/0x2f5c/0x2f5e]` at `0x3dd0–0x3e3a`. Extraction at `0x3daf–0x3dcf` reads the first two words. Later users can therefore see a different sequence, including the bomb shake at `0x167bd–0x16826`. With its own shake gate already active, that routine uses three calls to the same entry at `0x167d1`, `0x167e5`, and `0x167f9` to perturb the proposed X/Y and eye height. Power Surging can change those draws. It does not start the shake. One draw is consumed per surge pulse request accepted into an empty transient slot, with an additional draw on rearm even when another effect suppresses the pulse. With the same initial RNG state and deltas, the two draws on a rearm update occur in that order. No change to a specific enemy AI decision was demonstrated.

**EXE-confirmed presentation coupling.** The timer and battery update are independent, including when the charge reaches zero on the surge update. Battery depletion clears the optics flag first. The frame later chooses its base at `0x73e7–0x73ff`, so any selected surge tint on that update darkens the normal palette. Otherwise an active optic system continues to run and consume battery while the surge darkens its green output. The colored lamp lookup may still detect a lamp during either effect. Neither the lamp nor optics is a condition on the surge's RNG calls. The separate entry fade uses the selected source at `0x7490` and jumps over normal tint arbitration at `0x74dd`. Warning and audio requests come from the player update before that decision. See [optic system and battery](colored-lights.md#optic-system-and-battery).

**Port implementation.** The live session uses its shared gameplay counter,
retains the surge across travel and resets it on NEW GAME or continuation. The HUD message
queue deduplicates recurring notices. Surge and destruction requests share
the existing single BANK[14] loop. The pulse takes precedence over lamp tint
and darkens the already selected regular or green source at the world/HUD
upload boundary. Battery drain uses the same gameplay delta independently.
If charge expires on a surge frame, that frame darkens the regular source.
Blocking interfaces keep their own palette handling.

**Adapted.** The host uses an independent PRNG with the recovered range and
thresholds. The existing message queue displays the complete text immediately.
Competing DOS transient writers, including the original low-oxygen white
pulse, require their own port implementations. See [lamp presentation
differences](colored-lights.md#port-status-and-verification-boundary).

**Unknown / DOS observation:** Observe an ordinary pause/resume and level transfer near a surge boundary, actual audio competition with other BANK requests, and the order of visible warning glyphs versus darkness at varying frame rates. The branch ordering and ranges above are static findings. No DOS run was used to infer duration.
