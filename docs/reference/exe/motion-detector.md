# Enemy motion detector

Verified on 2026-11-23 against the `EXTREMIS.EXE` and `GAME.LAB` hashes in the [EXE index](../exe-index.md). `DATA/CASQUE.SPR` SHA-256 is `c8670b466a97df1aec097a1d8780e5a712494be5867927c28b7fd5ceb4f25349`. Addresses in brackets denote runtime storage. The other addresses are EXE file offsets, except where a resource file is named explicitly.

## Acquisition and sweep

- **EXE-confirmed.** Main-loop calls at `0x7026`, `0x7039` and `0x727a` update enemies, the player and projectiles before the detector call at `0x727f → 0x100f6`. The helper at `0x11335` gets the active level's record array from the level header word at `+0x0c` (`0x1137d–0x11383`). At `0x10115–0x1017e` the scan walks the live count and 28-byte enemy records in order. It skips health `0xffff`. The current sector at record `+0x08` selects its two origin words from `level+0x14+6*sector`, offsets `+2/+4`. It adds the current local coordinates at record `+0x0a/+0x0c` and subtracts the player's sector origin plus local XY (`[0x51da]+[0x51aa]`, `[0x51dc]+[0x51ac]`).
- **EXE-confirmed.** The rotation is `R=(180−heading) mod 720` (`0x10181–0x10191`, wrap at `0x3e67–0x3e7e`). At `0x11185–0x111ea` the lateral product is `DX*cos(R)−DY*sin(R)` and the forward product is `DY*cos(R)+DX*sin(R)`. In particular, `0x11196–0x111a4` subtracts the saved sine product from the cosine product. The Q14 conversion and the signed shift at `0x10196–0x1019d` give one detector unit per 64 world units. The 720-word Q14 tables at `0x3164` and `0x3704` and the 257-byte angle table at `0x3ca4` match the port tables, including the shifted cosine used for sine.
- **EXE-confirmed.** `0x101a0–0x101b6` accepts lateral X from −27 through 26 and forward Y from −3 through 31. The first ten accepted records fill the buffer at `[0x5463]` (`0x101bb–0x101e3`). Distance sorting, enemy movement and facing, visibility and door state play no part in this selection. Records with zero health can still qualify until marked `0xffff`.
- **EXE-confirmed.** At `0x13e85–0x13eef` the HUD computes `abs((phase&7)−floor(angle(−X,Y)/60)−1)` using the angle routine at `0x3ee8–0x3f6c`. Values 0–2 select progressively smaller markers. The fade uses the linear difference without wrap and runs after the ten-contact limit. At exact lateral bearings with Y=0, the left peak is phase 1 and the right peak is phase 7. Directly behind with X=0 and Y<0, the angle sector is 9 and none of the eight sweep phases draws a marker, even within the acquisition rectangle.

## CASQUE composition

**EXE-confirmed / DATA-confirmed.** `0x13e00–0x13e4c` copies the 46×29 radar background to `(0,171)`. Zero-based CASQUE index 4 is the off graphic and indices 5–12 are the eight sweep backgrounds. They contain none of the contact color indices 59, 60 and 131. At `0x13e5c–0x13e7f` the radar sets the inclusive draw rectangle to X 0–45 and Y 171–199. The contact routine at `0x13ecc–0x13ee7` hands the blitter the logical anchor `(X+24,198−Y)`. The blitter at `0x22ee` subtracts the SPR origin before clipping (`0x2314–0x2346`) and skips transparent index zero (`0x257d–0x2585`). Contacts draw in record order. Later opaque pixels can cover earlier dots.

| Zero-based CASQUE index | SPR image header offset | Size | Anchor |
| --- | --- | --- | --- |
| 13 | `0x7470` | 5×5 | (2,2) |
| 14 | `0x7491` | 3×3 | (1,1) |
| 15 | `0x74a2` | 1×1 | (0,0) |

## Port reproduction and correction

**Port-tested.** `AppSession` scans `activeEnemies->records()` after the enemy and weapon updates. The enemy system changes the sector and local XY together during a crossing and rebuilds the sprite scene from the same world position. In the original level 2, enemy 0 starts at world `(3700,1900)` in sector 4. From `(3572,1900)` facing +Y (90°), the old port plotted it at `(22,198)` with peak phase 1. The DOS expression yields `(26,198)` with peak phase 7, on the right. `motion_detector.cpp` now uses the DOS subtraction order. `original_hud.cpp` now subtracts the encoded SPR origin before clipping. Previously the 5×5 and 3×3 markers drew from their top-left corner and shifted right and down by two and one pixels respectively as the fade changed.

The original-resource test isolates one record at a time for pixel attribution while running the full enemy system on the stock level. It checks all eight phases and the complete 46×29 panel for 16 relative direction and heading combinations. From player world `(400,500)` in sector 10, enemy 11 moves from `(495,911)` in sector 11 to `(525,912)` in sector 10 at fixed step 24. Its local X changes from 495 to 125. At heading +Y both positions quantize to anchor `(25,192)`. The test checks moving contacts on both sides against the live render scene. The original-resource test failed on marker placement in the baseline and then on the mirrored sweep after the marker correction alone.

```sh
./build/debug/motion_detector_tests /path/to/EXTREMIS.EXE /path/to/GAME.LAB
```

Debug and Release CTest passed 45/45 each. The explicit original-resource runs also passed for the detector, battery, HUD, interactions, optics and palette effects. The separate original-resource enemy test reports `Native MONS5 movement/attack failed: 13` in both builds, in the level-3 movement/attack fixture. That test binary contains neither detector nor HUD code.

**Adapted.** DOS increments `[0x5423]` at `0x7387` each gameplay loop before the HUD call at `0x73ad`. The port advances the sweep from the 18 Hz enemy simulation counter and rounds its continuous camera pose to DOS position and heading units. DOS displacement uses signed 16-bit words. Port distance arithmetic is wider and rejects out-of-range input. The 30 stock levels fit within axis spans of 11,600 and 7,800 world units. The comparison uses original instructions and assets. A running DOS capture of the sweep cadence is pending.
