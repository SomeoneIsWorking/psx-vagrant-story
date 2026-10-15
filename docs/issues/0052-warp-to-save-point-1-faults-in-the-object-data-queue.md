---
id: 0052
title: Warping to save point 1 (location 15) faults inside BATTLE's object data queue
status: open
symptom: `warp 1` jumps to 0x004B0001 about 40 fields after the transition and the run aborts
tags: S013,debug,battle
created: 2026-10-15
updated: 2026-10-15
---

Observed: New Game (`4300:tap start 4;4600:tap cross 6;4800:tap circle 6`), then `9000:warp 1`. The log shows
`save point 1: location 15 (zone 9 map 6) at (1, 8) requested`, then `host dispatch to 0x004B0001 FAILED ... the
block that produced this target began at 0x8009969C` and `vs_main_execTitle tail stopped at 0x004B0001`. The same
fault follows `warp 0` then `warp 1` from free roam (`0x800F196C` = 1), so the event-script mode is not the cause.
Save points 0, 2, 3, 6 and 12 load their maps and reach free roam. Runs under `scratch/restructure/smoke/probe5`,
`probe6`.

Expected: location 15 loads as it does from the Teleportation spell.

Where: `0x8009969C` is in BATTLE.PRG `vs_battle_processObjectDataQueue` (`0x8009967C`, rood-reverse
`src/BATTLE/BATTLE.PRG/30D14.c`), the per-slot loader dispatch after `VSync(1)`. The cause is not traced: either the
location's object data reaches the queue malformed (a port CD/load defect) or the warp skips state the spell sets
first (`func_800735F8` calls `func_800A1108`, `func_8006F4B0` and `vs_battle_setStateFlag(0xB7, 1)` before
`func_8007C694`). Next step: run the same location from the in-game Teleportation menu, which settles which.
