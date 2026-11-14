# BATTLE replay interpolation (S011)

The BATTLE logic runs at 30 or 15 fps (`vs_gametime_tickspeed` `0x8005E24C` = 2 or 4 fields, issue 0039), so its
in-betweens come from replaying its draw with blended inputs (psxport `docs/plans/replay-interpolation.md`). The frame
driver already commits the fields of each frame's wait (issue 0054), which the presenter needs for an in-between slot.

## The frame (`vs_battle_exec` `0x800798A4`)
One iteration of its inner loop is one frame:

1. Pad (`vs_main_processPadState` `0x80043940`), the frame's OT `ot = 0x8005E210`-indexed buffer, `ClearOTagR(ot, 0x822)`,
   the scratchpad packet pointers (`0x1F800000`, `+4`, `+8`).
2. Logic: `0x8008C8A8`, the actor query `func_800A0BE0`, then the mode switch on `0x800F196C`
   (`func_8006E7F0`, `func_8006EBF8`, `func_8006E640`, cutscene steps).
3. The draw tail, from the `func_80069D78` call (camera angles from `vs_scratch.camera.position`/`lookAt`) and
   `func_8007ACB0`, `SetRotMatrix`/`SetTransMatrix(0x1F800014)`, the mode-2 step switch, the `0x800F190C` block
   (`0x8008EC48`, `0x8008AC78`, `0x8008B28C`, `0x800AEF94`, `vs_battle_processObjectDataQueue`, `0x800941FC`),
   `0x800CF060`, `0x800CA2DC`, `0x800CB83C`, the per-tick loop over `tickspeed / 2` (vibration and three resident
   calls), `_drawScreenTransition`, `0x8007D734`, `0x800E511C`/`0x800E5308`, `SetDrawStp`, `AddPrim(ot + 0x2080)`,
   `0x800E45B4`.
4. The present `func_8007629C(ot + 0x2084)`: flip, `DrawSync`, the wait in `vs_main_gametimeUpdate`, then
   `PutDispEnv`, `PutDrawEnv`, `DrawOTag`. The wait is the frame driver's step boundary, so the record sealed there
   holds the previous frame's chain; the section declares `submitsAfterWait` and its in-betweens go to the next
   drawing record.

## Built
- The section (`game/frame/draw_section`) replays the tail from `func_80069D78` up to the present and takes the OT
  head from `a0` there. Each replay starts from the tail's entry state, so the OT words and packet pointers the
  logic wrote are the real frame's.
- The camera track (`game/frame/draw_replay`): `vs_scratch.camera` position and lookAt blend linearly, roll
  (`angles.vz`) as a 12-bit angle; the sync derives the other angles.
- The texture animation step `0x80091B04` is the real frame's work (its `MoveImage` uploads the next texels), so
  replays skip it.
- Cuts (`game/frame/shot_cuts.h`): the event camera command `func_800BC2E8` kind 1 copies its target into
  `cameraPos` with no move, and `func_800BBE04` switches `vs_battle_applyCameraState` to the follow camera. Either
  declares a cut, latched at the next draw tail and reported for the record that walks that tail's chain. On the
  New Game intro the 7 declarations hold one interval each, and the two visible shot changes in fences 1888-2187
  (pad replay) are exactly those held intervals. `PSXPORT_DEBUG=vagrant-cut` logs each declaration.
- On the New Game pan (camera track moving every tick), the background edge of each in-between lies at the midpoint
  of its neighbours to within a quarter native pixel on average. A paced run shows 0.5 logic tick per vblank on the
  30 fps steps and 0.25 on the 15 fps ones. 25 of about 4400 intervals refuse: an object load's `LoadImage` and a
  sound path's `DisableEvent` inside the tail.

## Open
- Actors, their poses and effects draw at the newer tick in every in-between, so characters still move at the
  logic rate. Next: actor position and rotation tracks, then the pose (frame and progress as a unit).
- Other camera placements outside the event camera (a room change, `func_800BBE10`'s origin change) are not
  declared.
- Leak proof: with fps60 off the field digest logs once per 2-field step, 13.6k ticks past the boundary, and with
  fps60 on once per part at its boundary, so the two runs never print the same `ticks=` inside BATTLE and the
  on/off RAM comparison does not apply as written.
