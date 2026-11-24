# DOS keyboard controls

2026-11-23. Static trace of the local `EXTREMIS.EXE` with SHA-256
`7b2883d5fccdd75d75e4832cf55a82a46358ffb25d7c037b2fa12551ba32f397`.
Offsets refer to EXE file positions. Brackets denote runtime addresses.

## Turn direction

**EXE-confirmed.** The IRQ handler recognizes the PC keyboard `E0` prefix at
`0x2907–0x2913` and `0x2958–0x2968`. The lookup at `0x28b2–0x28d9`
uses the table at file `0x267c` (runtime `[0x247c]`). Its first four
pairs map the extended make codes `48/50/4b/4d` to `60/61/62/63`
(up, down, left, right). `0x292e–0x2953` sets or clears the corresponding
bit. The query at `0x2aa0–0x2ad0` reads that same bitset.

The built-in binding words at file `0x5611–0x5618`, corresponding to
`[0x5411..0x5418]`, are `0062 0063 0060 0061`. Startup copies the
corresponding bytes of a present `EXTREMIS.CFG` at
`0x6856–0x687a`. The local 13-byte CFG has arrow make codes
`4b 4d 48 50` for these four bindings in that order.

The first turn binding sets `[0x5403]` to +1 at `0x16d1e–0x16d3c`.
The second sets it to −1 at `0x16d3d–0x16d5b`. If both are held, the
second write takes precedence. At `0x16707–0x16725` the player adds
`12 * [0x5403]` to `[0x51de]` and wraps it modulo 720 via `0x3e67`.
The movement path at `0x1677f–0x167b6` uses the cosine and sine
tables at file `0x3164` and `0x3704` via `0x3ebb`. At heading 180
(facing +Y), adding 12 turns toward −X, which is screen left in the
original projection. Subtracting 12 turns toward +X, which is screen right.
See the [original lateral projection trace](motion-detector.md#acquisition-and-sweep).

## Other gameplay bindings

| Action | Evidence in the reference executable |
| --- | --- |
| Forward / reverse | `0x16d5c–0x16d99` writes +1 or −1 to movement word `[0x5405]`. |
| Shoot / activate | `0x16d9a–0x16db8` tests the configured key `[0x5419]` independently of movement. The default word at file `0x5619` is `0039` (Space). `0x17a64–0x17a72` gates the weapon on `[0x5407]`. `0x168b6–0x168bc` passes the same action word to world collision/interaction. |
| Run / crouch | `0x16db9–0x16df6` tests `[0x541b]` and `[0x541d]` separately. Default words at file `0x561b` and `0x561d` are `001d` and `0038` (Ctrl and Alt). `0x16726–0x16778` uses `[0x5409]` for running. `0x16a48–0x16a7d` uses `[0x540b]` for crouch. The local CFG keeps the same three action key codes at byte offsets 9–11. Its SHA-256 is `e066a2d7cfad2e8db3a2f53c029d1667757da82b610fa954790c6b2e229af716`. |
| Inventory / map | `0x16df7–0x16e34` samples `[0x541f]` and `[0x5421]`. The built-in words at file `0x561f` and `0x5621` are `0017` and `0027` (I and the physical key used for M on AZERTY). `0x6eda–0x7020` dispatches the two requests. |
| Pause | `0x6ca3–0x6cd0` polls make code `19` (P), clears the key and stops voices before entering the pause presentation. |
| Floor/ceiling detail and effects | `0x6e90–0x6eb4` clears F and toggles `[0x542d]`, which gates the floor/ceiling pass at `0x12ac5–0x12ad2`. `0x6eb5–0x6ed9` clears S and toggles `[0x542f]`. When effects are off, `0x12ed9–0x12ef1` stops voices and exits the queued audio update. |

**Adapted.** Classic in the port uses the requested Space crouch, left Alt
run and left Ctrl + Up action binding. It retains the traced arrow directions
and uses the port's movement timing and speed. A reachable panel takes
priority over firing. Enter provides the same gameplay action for systems
that reserve Ctrl + Up. Pause opens the port's existing pause menu. The
port's S shortcut mutes the output of live game effects while their playback
state advances. See [the port bindings](../../systems/player.md#controls-in-the-port).

**Port-tested.** `player_input_tests` covers forward and reverse, the signed
turn directions, running and posture, a quick keyboard tap reaching the
weapon, context priority, release/modals and profile changes. The EXE
findings above are static. DOS execution of customized bindings remains
unverified.
