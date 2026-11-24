# Ship self-destruction and refused evacuation

Investigation: **2026-11-16**, documentation only. Fresh, bounded 32-bit LLVM
instruction reads using `tools/inspect_original_code.py`; original data parsed
read-only. **EXE-confirmed** and **DATA-confirmed** below are not DOS observations.
No engine changes, cinematic re-analysis, or DOS timing measurement.

Independently checked SHA-256:

- `EXTREMIS.EXE`: `7b2883d5fccdd75d75e4832cf55a82a46358ffb25d7c037b2fa12551ba32f397`
- `GAME.LAB`: `acd5eb522ef45acdded0e062a5c94c6c78b69a93cf7c85d248cb1f7efcb61b2c`

Offsets are absolute **file offsets**; brackets mean runtime memory. Levels
are human 1-based, sectors zero-based. Static text pointers used below map to
file pointer+`0x200`; this is not a universal address conversion.

## Answer

Refusing either shuttle question does **not** kill the player or permanently
disable boarding. It returns them to level 22/sector 38 at local (300,900),
angle 0. The remaining countdown resumes in gameplay. If they stay until it
expires, the original selects death cause **AX=2**: white transition,
`VAIS_EXP.ANM` with explosion sound, then **GAME OVER and the main menu**.
Health/oxygen death takes precedence if either resource has already run out.

The exploding installation is the **ship**, not the shuttle or arrival base.
The original captain's message (bounded text at file `0x2294f–0x22d6f`)
describes destroying the infested ship and the central computer's protective
security system. This is **DATA-confirmed narrative**, not a simulated reactor
or stellar-collision mechanic. Runtime activation is specifically the sixteenth
successful security-panel deactivation. The successful evacuation presentation
and its ship explosion are inherited from [the completed travel analysis](elevator-cinematics.md).

## Activation and prerequisites

**EXE-confirmed:** these three pieces of state are different:

| Runtime storage | Meaning | Initialization |
| --- | --- | --- |
| bytes `[0x60a7..0x60b6]` | Sixteen per-security disabled flags | All zero, `0x6b10–0x6b1a` |
| word `[0x60b7]` | Number of accepted, distinct security deactivations | Zero, `0x6b1b` |
| dword `[0x60b9]` | Remaining destruction time | **48000**, `0x6b24` |

`0xa5ac–0xa5f9` dispatches interaction type 4. It indexes the security flag
by low byte of wall parameter minus 1. A nonzero flag produces the already
disabled message and skips the reader: repeating a panel cannot increment
the count again. Otherwise it sets `[0x90ee]=-1`, invokes the shared reader,
then restores that mode word to zero.

The reader takes security ID from the parameter's low byte and required card
ID from its high byte (`0x937c–0x9395`). Card selection/cancellation is handled
at `0x9447–0x948a`; the selected card's low byte is compared with the required
ID at `0x95a1–0x95a9`. On acceptance in security mode, `0x95e4–0x95f9`
sets that flag to `0xff` and **increments `[0x60b7]`**. It does not start a
new timer or write `[0x60b9]`. The displayed panel result is the security
number/deactivated message (text file `0x21d79`, numeric patch `0x9603–0x9610`).
Wrong-card branch `0x961d–0x9634` reports failure without changing these fields.
This is card-reader interaction, not entry of an autodestruction code.

**DATA-confirmed:** all sixteen `0x34xx` security-panel walls in the original
LAB are in **level 22, sector 43** (the 44th sector if counted from one).
Records are at `0x2df3c–0x2dfbb`, eight bytes each, walls 30–45, type `0x3422`.
Parameter words in wall order are:

```text
0309 0101 040d 0205 030a 0102 040e 0206
0207 030b 0103 040f 0410 0208 030c 0104
```

Thus IDs **1–4 require card ID 1**, **5–8 require 2**, **9–12 require 3**,
and **13–16 require 4**. There is no order test or separate final confirmation
in the acceptance code; whichever distinct panel completes sixteen starts
the active condition. Cards need not all occupy the three inventory slots
simultaneously. Previously completed panels retain their global flags.
The shuttle's **`0x2005` card is a separate requirement**, not the activator.
Reaching the panels and obtaining the cards remain campaign/navigation tasks;
no additional terminal, captain-message flag, zone code, or zone predicate
appears in the recovered counter activation test.

## Countdown, display, alarm and timebase

`0x16c57–0x16cc6`, reached through the sole direct gameplay call
`0x7039 → 0x1656a`, does the following:

```text
if word[0x60b7] == 16:
    request BANK[14]
    remaining = signed_dword[0x60b9] - zero_extend(word[0x1428])
    if remaining < 0:
        remaining = 0
        word[0x5429] = -1
    dword[0x60b9] = remaining
    update minutes/tens-of-seconds/seconds in the warning string
    queue that message
```

The test is **equality to 16**, not >=16. Normal interaction cannot exceed
sixteen because the shipped IDs are unique and repeated deactivation is gated.
There is **no level or sector condition** on countdown or expiry.

Formatting at `0x16c8a–0x16cbc` divides by **9600**, then the remainder by
**1600**, then its remainder by **160**. File `0x21b52` contains the short
warning `ATTENTION:` followed by `0M 00S UNTIL EXPLOSION!`; digits are patched
at file `0x21b5e`, `+3`, `+4`. Initial budget is therefore **5 minutes in the
game's own displayed units**. Subtraction precedes formatting: the first
rendered number need not actually be 5M 00S; integer division rounds down.

The warning uses message queue `0x14399–0x14414`, which deduplicates pointers,
not a separate numeric HUD instrument. Its ordinary renderer begins at x=8,
y=16 (`0x14415–0x14489`), with CR advancing y by 11. Other queued messages
can affect when the warning is presented. No new countdown bar was found.

`0x16c61–0x16c65` requests **BANK[14] on every active update** through
`0x13103–0x13122` (sets its request byte if audio is enabled), rather than
loading shuttle `ALARME3/4.DSS`. Parsed BANK[14] has volume 64, continuous=255,
priority=255, 5000 Hz, 4608 samples, loop flag 65535, bounds 0..4607.
Its original name field is dots; do not invent an asset name or claim a spoken
warning. The audible character and mixing against other sounds need DOS review.

The adjacent `0x16be0–0x16c52` is **not the destruction timer**. It independently
decrements `[0x608f]`, queues a warning (text file `0x21a90`), requests the same
BANK[14], and, if another palette effect is absent, sets a black RGB blend of
amount `(rng & 63)+30`. Below -640 it reloads `(rng & 65535)+9600`.
The surge runs independently of `[0x60b7]` and may darken the screen during
self-destruction. The expiry transition below supplies the countdown-specific
white effect. See [Power surge](power-surging.md).

**Temporal limits:** PIT divisor **7457** (`0x184d–0x1857`) implies nominal
~160 interrupts/s. The handler increments `[0x1428]` only while `[0x142c]==0`
(`0x18c6–0x18d8`). A nominal 48000 raw interrupts would be about 300 seconds,
but this counter is read during gameplay and separately reset to **1** at
`0x71d6`, with interrupt inhibition, modal resets and CPU scheduling between
reads. Neither five exact wall-clock minutes nor an exact measured active-play
duration has been established. The recoverable duration is **48000 counter
units / displayed 5:00**, not an unqualified stopwatch claim.

## Interruptions and persistence

The destruction value is updated by gameplay, **not by the interrupt handler**.
The following bounded caller/return paths establish suspension:

| Interruption | Relevant code | Countdown consequence |
| --- | --- | --- |
| Inventory | `0x6f29`, return `0x6f7d–0x6f98` | No update inside; reset tick word to 1 on return |
| Map | `0x6ffc`, return `0x7001–0x701c` | Same |
| Reader/terminal/elevator modals | `0x1698c → 0xa508`; `0xa617–0xa649`; `0x16991–0x169b2` | Reset ticks; accepted interaction returns before the timer tail |
| Explicit P pause / idle pause | `0x6cc7–0x6e87` | `[0x142c]=-1` until exit; blocking input loop |
| Escape return/quit confirmation | `0x7635–0x767c` | Inhibit tick word during modal; resume or leave gameplay |
| Other guarded blocking calls | `0x6c6c–0x6c84`, `0x7226–0x7234`, `0x75ff–0x760d` | Tick inhibition; not independent countdown threads |
| Level-22 recorded-message dialog | return `0xaf34–0xafb4` | Resets ticks to 1; no destruction decrement in dialog |
| Shuttle questions, error acknowledgement and whole journey | `0xb4b2–0xc86b`, return `0xc845–0xc84e` | No gameplay update; reset ticks, discard dialog elapsed ticks |

Pausing can postpone the explosion indefinitely in wall time, but is not
survival after an expired countdown. Rejecting a question resumes the existing
remaining value; there is no charge for all the seconds spent answering it.

Ordinary sector movement leaves all three security/countdown fields intact.
Level save/load (`0x7b83–0x7c40`, `0x791e–0x7b82`) operates on the LAB level
record, not these global variables. Ordinary elevator travel therefore keeps
flags, completed count and remaining time, with modal time suspended. Returning
to a lower level does not remove the eventual explosion death.

**Continue with party is password continuation, not a serialized countdown
snapshot.** `0x1dbc6–0x1dca5` matches an eight-character password to a fixed
42-byte record; `0x1de72–0x1defc` installs coordinates, zone/floor, inventory
and vitals. Both new-game and accepted continuation return through `0x6a4e`
to unconditional initialization `0x6b10–0x6b2d`: flags/count zero, time 48000.
The 16-entry table at file `0x1d600` includes a level-22 start record at
`0x1d7a4` (zone word 21), but no destruction state. This explains what loading
a native continuation preserves: **not the previous countdown or disabled
security flags**. The common initializer also creates a fresh working
`GAME.LAB` from `DATA/DATA.LAB` before loading the checkpoint. See
[checkpoint templates and level persistence](password-continuation.md#gamelab-during-and-between-sessions).

## Refusal, failure and retry

The already documented prompts/films are not re-investigated here. Their
relevant state boundaries were checked:

| Result | State afterwards |
| --- | --- |
| NO to enter | Common refusal exit, still level22/sector38 |
| YES to enter, NO to start | Interior fades out, same refusal exit |
| YES to start, missing `0x2005` in all three card slots | Error text file `0x22dc2`; acknowledge action, same refusal exit |
| Card present, completed count !=16 | Error text file `0x22dfb` says autodestruction is off; acknowledge, same refusal exit |
| Both prerequisites met, expiry not yet latched | Completed journey, level23/sector1; count set to zero before destination load |

Card test has priority over security test (`0xba4c–0xba87`); neither test
consumes a card. Error acknowledgement forces the refusal selection at
`0xbb07`; fade/exit is `0xbb10–0xbb32 → 0xc78f`.

`0xc78f–0xc7bb` sets local X=300,Y=900,angle=0 without changing sector/zone.
Common cleanup clears motion/impact transient words and six HUD damage-marker
records, frees presentation buffers, restores rendering and clears action
input (`0xc7bc–0xc86b`; details inherited from the travel note). Health, oxygen,
inventory, security flags/count and remaining time are not reset or damaged
by refusal. There is **no refusal-only movie, death, or menu transition**.

**Connection 45 is reusable.** Entry `0xb51d–0xb520` writes word zero, not
`0x0400` (locked) or a permanent consumed flag. Fresh checks at
`0xb4b2–0xb4ec` only require zone21, sector38 and an opening/nonzero phase
above 60. Ordinary movement can set `0x0100` again at `0x10a03–0x10a43`;
its filters `0x109a8–0x109cc` reject wall bit `0x0800` and lock `0x0400`,
both absent here. The door then uses the existing automatic phase updater.

LAB `0x2d7e8` is sector38/wall0: type `0x1017`, connection45, local segment
(200,1100)→(400,1100). Refusal position (300,900) is 200 units from its midpoint,
outside the ordinary 180-unit opening threshold; approach it again while facing
it. The paired wall is sector45/wall0 at LAB `0x2e21e`, same type/connection.
The original connection at LAB `0x29e40` is also zero. **Static conclusion:**
zero is a normal closed, unlocked state; there is no permanent lockout caused
by refusing. Exact walking/input reproduction remains **DOS-unverified**.

## Expiry and successful cancellation

`0x16c78` branches on signed nonnegative subtraction. Exactly zero is stored
and displayed without setting the exit flag on that iteration; a subsequent
positive decrement produces the clamp and `[0x5429]=-1`. `0x75bb–0x75c4`
then takes the end-of-game dispatch. At `0x76a9–0x76f4` health zero selects
AX=0 first, oxygen zero AX=1 next, then **count==16 AND time==0 selects AX=2**.
Merely seeing 0M 00S is not sufficient to identify the dispatch iteration.

Fresh read of the previously unexpanded AX=2 branch:

1. `0x1724a–0x17253 → 0x175bf`: stop voices, blend frozen current palette
   toward white (63,63,63), amounts **0,4,…,100**, with retrace waits
   (`0x175c4–0x175f8`). No gradual health drain is used to cause this death.
2. `0x17627–0x1766d`: start **BANK[29], voice0**, then `VAIS_EXP.ANM`,
   interval argument **11**, white reveal and no callback. The established
   decoder report gives **60 frames**; not re-decoded this task. BANK[29]'s
   30509-sample, 5000-Hz non-looping PCM matches original `EXPLO.DSS` byte
   for byte (PCM SHA-256 `489e90fc11a917def5db90675d064a80acb7989a7932b9e77dd5715f464fe3a0`).
3. `0x17670–0x1769b`: reset tick word, wait until **>160**, stop voices,
   fade brightness **100,98,…,0**. This is not another shuttle journey.
4. `0x176bb → 0x176db`: common GAMEOVER presentation, then return AX=0
   (`0x176cf`), caller `0x7700 → 0x6a40` main menu. Common text/input/timeout
   details are inherited from [player death](player-death.md).

For a normal successful evacuation, countdown execution stops as soon as the
blocking shuttle prelude is entered; selecting YES is not the reset instruction.
The **explicit disable is much later**, `0xc76d`: set `[0x60b7]=0`, after all
scenes and arrival coordinates, immediately before `0xc77f → 0x791e` loads
level23. **`[0x60b9]` retains its unused remainder**, and the sixteen disabled
bytes remain set. A blanket security/inventory reset would be incorrect.
The zero count prevents both further subtraction and explosion dispatch.

**Last-update edge (EXE-confirmed flow, DOS reachability untested):** movement
and countdown run at `0x7039` before the shuttle test at `0x7051`, whereas the
exit flag is tested at `0x75bb`. Shuttle eligibility does not test remaining
time or `[0x5429]`. If expiry is already latched in precisely the iteration
that triggers boarding, refusal returns to the normal explosion dispatch.
If the player instead completes evacuation, count becomes zero but the exit
flag remains set (no clearing write in shuttle or level loader). The subsequent
dispatch, assuming positive health/oxygen and no victory flag, skips AX=2 and
falls into `0x7705–0x77c3` application cleanup/exit, **not ordinary playable
level23**. This narrow control-flow anomaly needs a DOS trace; do not promise
that boarding after expiry reliably rescues the player or generalize it to
ordinary refusals. If subtraction merely equals zero without latching expiry,
the normal successful branch can still disable the countdown.

## Can it be avoided, stopped, or escaped another way?

- Do not complete the sixteenth security: countdown remains inactive, but
  the shuttle refuses departure for the same reason. No progress victory here.
- Once active, no re-enable-security/cancel/restart control was found in the
  recovered writers. Repeated readers cannot lower the count or reload time.
- Retrying boarding with the required card and count is the normal escape.
  Rejection itself does not strand the player permanently.
- Level22 has an ordinary elevator (sector0/wall6, LAB `0x2a3b2`, lift18).
  Its established route returns to level21/sector1, subject to the existing
  lift-card permission (card8). This is a retreat, **not escape from the
  explosion**: neither travel nor the global expiry branch exempts that level.
- The previously recovered route table has no elevator connection from the
  levels1–22 group to levels23–30; the shuttle is the recovered campaign bridge.
  No safe sector, alternate survival movie or second legitimate evacuation
  route is supported by these code/data paths. Full collision exploits and
  campaign reachability have not been proven by a static graph or DOS run.
- Password continuation/new game restarts the security state as above; it
  is not surviving the same active countdown. A modal can postpone time only.

## Port comparison and verification limits

**Historical inspection (superseded by the implementation update below):**
`src/item_session.cpp::usePendingCard` stores `securities_[16]` after
accepted readers; `resetPlans` clears them. The inspected port has no exposed
completed-security/countdown state, shuttle trigger or AX=2 death implementation.
Conventional A_LEVEL elevator presentation was subsequently integrated on
2026-11-16; see the updated travel note. Shuttle/countdown status remains unchanged.
These findings specify the original only; no implementation or new gameplay
test was made, and historical test counts are not presented as current results.

Validation consisted of hashes, bounded instruction reads, direct-address
reference scans, all 30 LAB records scanned for the security panels and local
door/elevator data, and original DSS parsing/PCM comparison. Direct EXE operand
occurrences for `[0x60b7]`: `0x6b1e,0x76e0,0x95f6,0xba77,0xc770,0x16c5a`;
for `[0x60b9]`: `0x6b26,0x76e9,0x16c6b,0x16c86` (operand locations, not
instruction starts). The indirect sixteen-byte initialization was separately
checked. These bounded checks found no timer reinitialization at activation,
sector changes or refusal.

Remaining **DOS verification**: wall-clock active-play duration and display
queue timing, audible alarm/mixing and white/explosion playback, repeat boarding
by walking from (300,900), final-tick boarding anomaly, and end-to-end card and
security progression. No unresolved timer identity or refusal lockout should
be inferred from those dynamic checks still being pending.


### Port implementation update (2026-11-16)

The historical absence above is superseded: the local `SelfDestruction`,
`ItemSession::updateSelfDestruction`, gameplay dispatcher and AX=2 presentation
already implement the countdown/expiry path, visually validated by the user
before this task. This integration does not change that death presentation.
`ItemSession` now also retains the distinct completed-reader count required by
shuttle eligibility. After successful movie completion `finishEvacuation()`
sets it to zero and deactivates the same countdown object, without clearing
security flags, unused time or a previously latched expiry. No second timer or
card store exists. Refusals retain all of these values and resume gameplay.
See [campaign integration and Debug procedure](elevator-cinematics.md#campaign-shuttle-integration-2026-11-16)
for implementation, tests and limitations. All original-game findings here are
inherited; this task performed no further EXE investigation. The final-tick
boarding anomaly remains pending, without a new policy or claim of rescue.
