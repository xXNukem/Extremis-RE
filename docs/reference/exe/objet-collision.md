# OBJET collision and enemy sector transitions

## Reference and evidence

Verified 2026-11-19 against `EXTREMIS.EXE` SHA-256 `7b2883d5fccdd75d75e4832cf55a82a46358ffb25d7c037b2fa12551ba32f397` and `GAME.LAB` SHA-256 `acd5eb522ef45acdded0e062a5c94c6c78b69a93cf7c85d248cb1f7efcb61b2c`. Code and table locations are absolute file offsets. Bracketed addresses denote runtime memory.

| Confirmed rule | EXE evidence |
| --- | --- |
| Scan the selected sector's eight-byte records in order | `0x10dbd–0x10dfc` selects the sector from the caller's word and reads the vertex. `0x10fc8–0x10fe1` advances to the next record. |
| Skip type bit `0x2000`, use the low byte as the current image, require bit `0x1000` to block | `0x10e00–0x10e1e` indexes the uint16 table through `[0x5b1f]`. `0x10e95–0x10e9d` gates physical response. The 243 words at `0x5d4f, 0x5da5, 0x5e09, 0x5e7f, 0x5ed7, 0x5f31, 0x5fa5, 0x6009, 0x606d, 0x60c9` match all ten port tables. |
| Candidate-point test in XY, lower bounds included and upper bounds excluded at campaign coordinates | `0x10e21–0x10e8f` adds and subtracts one half-extent on both axes and branches on word subtraction sign. An extent of zero gives an empty interval. The routine does not read height, anchors, pixels, or either parameter for physical blocking. |
| Choose a face using previous Y before X | `0x10f3b–0x10fbd` checks Y upper, Y lower, X upper, then X lower. The next record sees the corrected candidate. A proposal outside the rectangle is not swept through it, and an inside start without an eligible face is not pushed out. |
| Enemy turn by ±80 in a 720-unit circle | `0x10ff7–0x11028`, angle difference `0x3e9e` and wrap `0x3e67`. The actor mode of zero skips the turn. |
| Player: OBJET in the source sector, then walls/connections | `0x16851–0x16888` passes the current sector, previous and proposed XY, and mode 0. `0x168e8` calls the wall/connection routine. |
| Enemy: walls/connections, then OBJET in the returned sector | Walking `0xe650→0xe682`, MONS5 forward `0xf01c→0xf04e`, MONS5 recoil `0xeb11→0xeb43`. The wall routine rebases the candidate in `0x10d66–0x10db0`. Each OBJET caller stores the returned sector and still pushes the old record's XY words, with mode `0xffff`. Thus previous XY retain their source-sector values when the destination has a different origin. |
| Navigation and animation inputs | Polygon routing `0xf627–0xf723` and sector routing `0xf724–0xf838` use authored links. OBJET is checked during the resulting movement. The collision reads the live low type byte. The OBJET6 writer at `0x11e79–0x11ea5` changes that byte while preserving the flags, and frames 13–22 all have zero extent. The other identified whole-image cycles also retain their extent. |

The nonblocking OBJET1[2–3] branch at `0x10ea3–0x10f36` writes a player colour effect. The actor path at `0xff95` concerns death fragments. Both are separate from the player/enemy physical-response change.

## Campaign inventory

**DATA-confirmed.** The 30 levels contain 971 OBJET placements, 192 distinct bank/type-word pairs and 157 distinct bank/image pairs. All placements resolve to original scene identities `(sector, entity)`, including flips encoded above the low image byte. The campaign selects nine banks. OBJET3 has an EXE extent table but no placements in this `GAME.LAB`; its SPR is absent from the local installation. All placed sprites have available pixel dimensions, anchors and height-table entries. Param1 ranges from 0–256 and Param2 from 0–800 in these records.

| Bank | Levels (1-based) | Solid | Passable | Image indices observed solid | Image indices observed passable |
| --- | --- | ---: | ---: | --- | --- |
| OBJET1 | 1, 5, 8, 11, 15 | 110 | 61 | 0, 4–6, 9–11, 15–21 | 2–3, 7–8, 12–14 |
| OBJET2 | 2, 4, 9, 12, 14, 20 | 185 | 61 | 0, 4–12, 14–15, 17–19 | 2–3, 7–11, 13, 16, 20 |
| OBJET3 | — | 0 | 0 | — | — |
| OBJET4 | 23, 28 | 78 | 22 | 0, 2–4, 6–7, 9–10, 17–21, 26 | 5, 8, 12–16 |
| OBJET5 | 3, 19, 21, 22, 24 | 120 | 0 | 0–13 | — |
| OBJET6 | 7, 10, 13, 17 | 100 | 47 | 0, 7–10, 23–30 | 2–6, 11–13 |
| OBJET7 | 25–27 | 44 | 16 | 0, 5–8, 11, 13–14, 17–21, 26 | 2–4, 10, 16, 22–24 |
| OBJET8 | 6, 16, 18 | 77 | 32 | 0, 2–7, 9–10, 17–22 | 8, 11–16 |
| OBJET9 | 29 | 1 | 0 | 23 | — |
| OBJET10 | 30 | 17 | 0 | 0–7, 10–12, 15–18 | — |

Solid/passable counts are **inferred from the confirmed EXE rule applied to the recorded flags and extent**, and checked in the port. Nine `0x5xxx` instances have zero extent. In OBJET2, indices 7–11 include both solid and passable instances. The full type word is required to classify a placed record. Enemies are loaded from the separate enemy table. Projectiles, hit effects and non-placed images used as animation patch sources are outside these counts.

## Port comparison and regression

**Port-tested.** In level 22, sector indices 10→11, wall 0 (type `0x1017`, connection index 11), the original geometry admits the controlled candidate (11250,2120) from (11250,2090) with a passable connection. The destination has OBJET5[0] type `0x5000` at (11100,2300), half-extent 200. Old and destination origins are (10800,1000) and (11000,2000). The former port rebased the previous Y to 90. It resolved to world Y=2100. The EXE caller calculation keeps old local Y=1090, so it resolves to world Y=2500. Both resolved positions locate in the destination. This uses the original map and a controlled candidate, with no claim of observing that route in running DOS.

[`constrainEnemyObjets`](../../../src/extremis/world/objet_collision.cpp) now expresses the stored old actor words in the destination's box coordinate frame before calling the common resolver. Same-sector contacts, movement fractions on the unclamped axis and wall/door order remain covered by player/enemy tests. The test suite exercises normal and MONS5 movement, closed and passable portal geometry, solid and passable variants, multiple records, tangency and live frames. The original-resource player collision test checks both actor responses for all 971 isolated placements.

**Unknown / adapted.** Dynamic DOS observations are pending. The port retains continuous player movement and its existing wall/polygon solver. The EXE uses integer proposals and word tests, and parity at extreme overflow or at sub-unit contact boundaries is unverified. MONS5 recoil's original wall mode `0xffff` skips the connection passage branch at `0x10929–0x10931`; the port's recoil sector-change policy is outside this OBJET correction.
