# 0059 — Chained save-point warps exhaust the high heap

Status: open (2026-11-17). Owner: `game/frame/location_warp.cpp:LocationWarp::apply` and the BATTLE zone loaders it
starts (`func_8007C694`).

## Observed
From the prologue (frame 17100), `warp 0` .. `warp 12` taken 160 frames apart, the next warp aborts the run: with the
disk-load owner the zone read lands at `0x00000000` (LBA 14085, 196608 bytes) and overwrites resident code
(`scratch/caps/after/repro/run.log`: `[cw]` stores at `0x80028E80` from `processCdQueue`, then an unknown instruction
there); the binary from before that owner faults the same warp with an ambiguous code-image dispatch at `0x97597244`
(`scratch/caps/before/repro/run.log`). The first survey failed the same way at warp 11 after warps 0-10; a chain from warp 13 failed at its third warp
(15, `scratch/caps/after/rain_a`) and one from warp 24 at its tenth, so the count of warps is not the trigger alone.

## Cause so far
The zone loaders (`loadZnd`, `loadMpd`, the ZUD loader in rood-reverse BATTLE `146C.c`) enqueue
`vs_main_allocHeapR(size)` as the destination, and `vs_main_allocHeapR` returns 0 when no free block fits, so the high
heap is exhausted by then. Retail reaches a save point's Teleportation only from the menu, after the room's own
teardown; the warp calls the transition at any idle BATTLE field, so a room's loads may never be freed.

## Next
Trace the high heap (`vs_main_allocHeapR`/`vs_main_freeHeapR`) across one warp and find which allocation outlives it;
make the warp wait for the teardown the menu route performs, or refuse while it is pending.
