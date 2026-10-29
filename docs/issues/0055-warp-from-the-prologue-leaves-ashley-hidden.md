---
id: 0055
title: A save-point warp taken during the prologue leaves Ashley hidden and frozen in free roam
status: open
symptom: after `warp <n>` from the New Game prologue the map loads with its enemies, HUD and minimap, but Ashley is not drawn
tags: S013,debug,battle,warp
created: 2026-10-29
updated: 2026-10-29
---

Observed: New Game (`8600:tap start 4;12000:tap cross 6;12600:tap circle 6`), then `warp 3` at 14000 or 15500
(`scratch/readme-media/probe1`, `probe5`). Save point 3's room loads and Ashley's actor is placed at the target tile
(`D_800F4538[0]` = `0x801203A4`, position `(0x640, 0, 0x640)` for tile (12, 12)), but its flag byte at `+8` reads
`0x94`: `visible` (bit 1) clear and `freeze` (bit 2) set, 2000 steps after the warp. Save points 0, 2, 4, 5, 6 and 12
show the same: no Ashley on screen.

Expected: Ashley stands at the save point, as after the Teleportation spell.

Cause: during the prologue the event script drives puppet actors and keeps actor 0 hidden (`+8` = `0x90` at 13900
and 15400, `visible` clear). `LocationWarp::apply` (`game/frame/location_warp.cpp`) calls the location transition
`func_8007C694` from inside that event, so the transition carries the hidden actor across; the spell is only castable
in free roam, where actor 0 is already visible.

Proper fix: arm the warp only once the game is in free roam with actor 0 under player control (the prologue's own end,
or a loaded save), and refuse it during an event, instead of writing the actor's flags. Until then the README's
free-roam stills are left out; its stills come from the prologue cutscenes.
