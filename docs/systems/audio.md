# Audio: four voices and a lot of personality

The original game has a proper sound system. It's not a collection of WAV files you can fire and forget. Sample formats, looping, voice ownership and the timing of the main loop all matter — especially when a door sound gets stuck or footsteps overlap.

## What's in the bank?

The current audio reconstruction identifies **30 original effects**, played as **5,000 Hz mono** data through a **four-voice** mixer. The engine reads the original resources at runtime and converts/mixes them into the output expected by SDL3.

The interesting code lives in [`audio_data.cpp`](../../src/audio_data.cpp), [`audio_mixer.cpp`](../../src/audio_mixer.cpp), [`audio_player.cpp`](../../src/audio_player.cpp) and [`game_audio.cpp`](../../src/game_audio.cpp).

| Sound/effect | Known reference | Note |
| --- | --- | --- |
| Footsteps | `BANK[7]` | Part of the original movement/camera cadence |
| Door movement | `BANK[27]` | Audio lifecycle matters; don't leave a loop stuck |
| Player death (health) | `BANK[25]` | Should finish rather than repeat forever |
| Player death (oxygen) | `BANK[26]` | Separate death cause/effect |
| Interface machinery | Several bank entries | May start/stop across modal animation phases |

Sound numbers are references to recovered bank entries, **not** generic audio file names. A name that sounds like a breathing effect doesn't prove it is used whenever oxygen drops.

## What went wrong with doors?

One of the early bugs was an audio sample that played correctly at first but left a little fragment hanging at the end. The issue wasn't “we need a better sound”: it was a **voice lifecycle / loop termination problem**. The fix came from inspecting how the bank's DSS sample describes looping and when the door/panel routine should release it.

That's the kind of bug that is best caught with a tiny repeatable test: start the effect, let it finish, reopen the same interface, and inspect requests, completions and unintended wraps. The port has effect statistics to help make those checks.

## Two different clocks, and why they matter

The DOS programs a **PIT divisor of 7457**, giving roughly **160 interrupts per second**. Some game timers and resource consumption use that clock. Other animations and sound requests are associated with **main-loop iterations**, which have a variable DOS speed. The port's fixed 18 Hz gameplay step is a practical adaptation, not a declaration that all sounds historically ran at 18 Hz.

Footsteps are a good example: the original camera bounce has a phase table and can request `BANK[7]` on reaching a boundary. The local work restores that relationship while preventing duplicate distance-triggered step sounds. The exact overlap/voice behaviour still has small differences from DOS.

## Silence is sometimes the correct result

The original executable checks different events separately. We found no evidence of a universal “health lost → play sound” or “oxygen just decreased → play breathing loop” rule in the investigated branches. Health depletion and oxygen depletion eventually have distinct death effects, but that doesn't authorize adding warning noises to every decrement.

Likewise, **ambient machinery**, map-transfer sounds and other per-level cues still need controlled verification. Not hearing something in one play session doesn't prove the DOS never requested it.

## Reconstruction strategy

1. Read the original sample data and its looping metadata.
2. Trace where DOS requests or stops the sound.
3. Keep effect ownership in the game system that triggers it.
4. Test a full lifecycle, including repeated openings and interruptions.
5. Only then compare timing and subjective sound against DOS.

For the user-facing display that triggers many of these effects, see [Interfaces](interfaces.md). For player health/oxygen timing, see [Player](player.md).

**Current limitations:** a few audio tests deliberately exclude some unconfirmed ambient behaviours; exact voice overlap and some modal sounds remain open. The port does not bundle the original samples.
