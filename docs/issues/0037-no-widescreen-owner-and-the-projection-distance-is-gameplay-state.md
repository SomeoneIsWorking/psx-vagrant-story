---
id: 37
title: BATTLE widescreen draws on the record canvas; the projection distance is gameplay state
status: partial
symptom: S010 (true widescreen) needs the room geometry drawn into the canvas margins without moving the projection word
tags: S010,S015,widescreen,projection,decomp
created: 2026-09-27
updated: 2026-11-17
---

## What was built

`vagrant::BattleProjectionOwner` (`game/render/battle_projection.{h,cpp}`), registered through the
per-Core `psx::cpu::NativeDispatcher` from `vagrant::Runtime::admitExecutable` — the only boundary at
which the resident image exists. `vagrant::Session` holds the owner per Core.

It owns the four **resident** SDK leaves, not the overlay call site: BATTLE's presenter
`0x8007629C` re-states a literal `SetGeomOffset(160, 112)` every field, so an owner on the overlay's
call would be overwritten 60 times a second. The leaves are `SetGeomOffset 0x80041540`,
`SetGeomScreen 0x80041534`, `SetDefDrawEnv 0x8002B374`, `SetDefDispEnv 0x8002B434`.

It refuses rather than guesses, in four places, each with a positive test:

1. the GTE control register and the framework's record of the leaf's argument must agree;
2. the leaf's recorded value must equal its own argument register;
3. the guest's own viewport rectangle must agree with the width being published;
4. registration resolves every leaf against the image catalog before installing any, and refuses an
   identity this Core did not publish (BATTLE, TITLE and ENDING all reuse `0x80068800`).

The earlier `derive()` arm (a pure wide-centre derivation) was removed: under the record canvas the guest keeps its
retail centre and draw width, so nothing derives a wide projection.

## How the widening works now

`vagrant::Runtime` declares `RenderPath::Record` and a `GuestWidescreenProjection`; psxport's record canvas holds
`presentationHorizontalMargin` extra columns on each side of the displayed buffer and draws a primitive into them
when its draw area spans the buffer. The guest still projects with centre 160 and a 320 clip, so the only thing that
stops a margin quad is the guest's own screen reject. BATTLE's room draw is
`0x8008AC78` -> `0x8008B1FC` -> `0x8009723C` -> `0x80097388` (quad loop), and every screen test in it goes through
the register-convention routine `0x80098014`, which rejects a quad whose four projected vertices lie wholly outside
x [0,320) or y [0,224). `game/render/battle_cull.{h,cpp}` replaces that routine on the BATTLE generation with the same
test over a box grown by the canvas margin; rows stay retail. At 4:3 the margin is 0, so the box is the retail one.
The verdict only decides whether a packet is emitted; no actor, camera or projection word reads it.

Evidence (S013 route, field 5000): at 4:3 with the enhancement off recordcheck reports `mismatched=0` on all 4868
checked fields; at 16:9 both margins show the room (shots `m_wide/f05000`, `w6/f05000`); `0x8005E248` is `0xE0` in
both runs; over 98 aligned steps 66 are bit-identical in RAM outside the two packet buffers, timers and dead stack
slots, and the rest differ only in the 3-field phase blocks at `0x800F2458..0x800F2540` and `0x800F3DEC..0x800F3F8C`
because the two runs sample different fields of one game frame.

## What the decompilation census established (bounded claim)

| word | writes | reads | the readers |
|---|---|---|---|
| `D_8005DFD6` (published width) | 2 | 1 | `func_800BB874`, a screen-space `LINE_F2` scanline effect — **not a cull** |
| `vs_main_projectionDistance` | 4 | 14 | camera transition, two threshold branches, zoom, fog, menus |
| `vs_main_nearClip` (`0x8005E0C8`) | 2 | 3 | camera transition only |

No screen-space horizontal cull was found in the decompiled part; the room renderer is largely
undecompiled, so this is a bounded claim, not an absence claim.

**The real hazard is gameplay.** `vs_main_projectionDistance` (`0x8005E248`, setter `0x8007CCF0`) is
branched on against 272 and scales the GTE fog. A widening that raised the global would flip a
gameplay decision, so this owner widens the *canvas* and never `H`, and the threshold is a shipped
constant rather than a comment.

## Fixed-layout screens stay 4:3

`vagrant::AspectPolicy` (`game/runtime/runtime.cpp`) holds 4:3 while BATTLE is not the published overlay: the
splashes, the intro movie, the title menu and the NOW LOADING card are 320-wide pictures with other VRAM beside them,
and at 16:9 the movie showed that VRAM (title-screen tiles, a ghost of the logo) in its right margin
(`scratch/caps/before/route/f1000.png`, `f9000.png`). The attract demo runs BATTLE and the guest then reloads TITLE
through `_loadTitlePrg` `0x80041F34`, which nothing published, so BATTLE stayed the resident overlay on the second
movie; `_loadTitlePrg` is now owned and publishes TITLE (`readAndLoadTitle`), retiring BATTLE's generation. After:
the movie is pillarboxed at frames 800, 1000 and 9000 (`scratch/caps/after/splash2`, `after/route3/f9000.png`).

## BATTLE's GIM cards

The prologue staff credits are BATTLE GIM cards (`loadGim`, script opcode 0x17, drawn by `func_800CCA90`): a picture
in the centre and, when the card has a background colour, tiles of that colour over the rest of the 320x240 screen
(`func_800CBBCC`). Nothing draws beside them, so at 16:9 the margins showed other VRAM (`scratch/caps/after/route4/f15600.png`,
margin luma up to 204). `game/render/battle_cards.cpp` reports a visible, loaded, drawn card with a background
colour and `AspectPolicy` holds 4:3 while it is up; after, the same frame is pillarboxed with both margins at luma 0
(`after/repro/f15600.png`), and the world on either side of the card (f15500, f15700) stays 16:9.

## Sky dome and flat backdrop

The room backdrop `0x8008EC48` (rood-reverse upstream `146C.c`) draws one of a tiled picture, a flat fill (all five
room sky colours equal) or the sky dome `0x8009820C` (upstream `2EA3C.s`): 34 vertices through `rtv0tr`, then 64
gouraud triangles dropped when all three corners lie past one edge of x [0,320) or y [0,224).
`game/render/battle_sky.cpp` replaces the dome natively with the same transform, writes and packet chain, the reject
box from `battle_cull::widenedBox`; the flat fill's POLY_FT4 is moved to the widened box after the original runs. At
4:3 both are the retail bytes. On the manor's establishing shot the dome now fills both margins
(`after/route3/f12860.png`), where before only the triangles that straddled the 4:3 edge showed.

## Weather particles

Rain `0x8008F440` draws a falling drop only when it projects inside x (0, 320), y (0, 240); a native pass drawing the
margin drops from a snapshot of the pass's inputs was tried and removed: on the manor shot it drew one streak, and
of the 35 save points only 13 and 14 have particles (`0x800F1DC0` = 128), which are the other system
(`func_8008F9A4` seeds the same `0x800F1DD4` array as square flakes) and stop at the 4:3 edge, the right margin
empty (`scratch/caps/after/rain_a/w13.png`).

The flake pass is `func_8008FDC4`, not a drawing of state it already keeps: a flake it draws inside x (0, 320) drifts
on the next tick and calls libc `rand()` (shared with BATTLE's combat code, `74088.c`), while one it leaves undrawn
falls 32 times faster and respawns through `func_80090A1C`, which also calls `rand()`. Its visibility test drives the
flakes' motion and the RNG, so the game's own state holds no faithful margin flake, and widening the test would
change gameplay. Faithful margin flakes need the game itself to simulate them; the margins stay without flakes.

## What remains

- Not widened: the tiled 2D backdrop `func_8009147C`, the scanline effect `func_800BB874`, weather particles (rain and
  the flake system above), other particles, and the HUD (fixed layout by G003).
- These culls are screen-space draw decisions; none was found to feed gameplay.
