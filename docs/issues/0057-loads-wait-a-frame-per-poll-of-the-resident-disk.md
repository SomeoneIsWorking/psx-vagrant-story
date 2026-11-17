# 0057 — Loads wait a frame per poll of the resident disk

Status: partial (2026-11-17). Owner: `game/cd/disk_loads.cpp` (`processCdQueue`, `loadFile`).

## Observed
New Game shows the NOW LOADING card for over 200 frames before the manor's establishing shot fades in
(`scratch/caps/before/route`, card at 12700 to 12900, picture fading in by 13020), and a save-point warp (`warp 3`
at 14000) holds black for 116 frames between the transition's start (`vs_battle_screenTransitionStep` `0x800F1A48`
= 1) and its load completion (step 3), 133 frames to step 0.

## Cause
The resident CD queue (`_processCdQueue` `0x80044C74`, called from `vs_main_gametimeUpdate` after its VSync) starts
one queued file per call, and only when the disk is idle; the file's slot becomes loaded on a later call, and the
caller polls it. `vs_main_diskLoadFile` `0x8004493C` polls the disk state with `vs_main_gametimeUpdate(0)` until the
last sector lands. psxport's `StockVSync` ends the field on every VSync with a non-negative count, so each poll costs
a frame and each file several, while the disc itself is instant.

## Fix
`cd::processCdQueue` reads every queued file, whole sectors as `DsGetSector` delivers them, in the retail priority
order (each pass is one retail call's pass: Reading becomes Loaded, the priority-0 file is read, the others move up)
in the call that finds the disk idle with no buffered mode, then runs the disk state machine `0x800443CC` as retail
does. `cd::loadFile` opens the read with the retail `_diskInitRead` (its fields, `cdLoc`, state 5), delivers every
sector and leaves the read callback's end state (sectors counted and delivered, disk idle). A busy disk (XA, a direct
read) or a buffered mode runs the retail bodies. The payload and the queue's terminal state are retail's; what goes
is the frames spent polling.

## Proof (unpaced, 16:9, `scratch/caps`)
| route | before | after |
|---|---|---|
| New Game: NOW LOADING card | shown 12660 to past 12900, manor picture by 13020 (`before/route`) | never drawn; the title fades to black and the manor picture is up by 12780 (`after/route4`) |
| `warp 3` at 14000: step 1 to step 3 (load done) | 116 frames (`before/route/run.log`) | 47 frames (`after/route3/run.log`) |
| `warp 3`: step 1 to step 0 (arrival under control) | 133 frames | 64 frames |

## Left
- The 47 frames include the transition's own fade to black (alpha by `speed` 8 per frame, 32 frames); the rest is
  BATTLE's own per-frame load steps, not traced.
- The fades of a room transition are the game's; their Start skip is not owned.
