# 0058 — Event scenes and room fades have no Start skip

Status: open (2026-11-17). Owner: none yet (BATTLE's event script runner and `vs_battle_screenTransitionStep`
`0x800F1A48`).

## Observed
On the New Game route the manor establishing shot, the prologue's in-engine event scenes with their staff credits
(`scratch/caps/after/route4`, frames 12780 to past 17000) and every room transition's fade out and in play through
with no input; Start does nothing during them. Retail has no skip for any of them.

## Covered
The TITLE screens: the publisher logo takes the developer logo's own skip into its fade-out
(`game/render/title_splash.cpp`), the developer logo keeps retail's, and Start skips the intro movie.

## Next
Find a terminal transition each screen owns: the event script's end opcode for an event scene, step 3 of
`vs_battle_screenTransitionStep` for a room fade. A screen with none stays unskippable and is listed here.
