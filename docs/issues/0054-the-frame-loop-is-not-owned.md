# 0054 — The frame loop is not owned

Status: open, cadence owned (2026-10-28). Owner: `game/frame/frame_driver.cpp:FrameDriver::stepFrame`.

## What the code does
Each step commits the fields of the guest's last wait, then the resident/TITLE/BATTLE phases resume until the guest's
VSync `0x8001F6C4`; the per-frame wait is `vs_main_gametimeUpdate` `0x8004261C` (VSync at `0x8004262C`, called by
BATTLE's `func_8007629C` with `vs_gametime_tickspeed`). C++ main is `0x8001F5EC`.

## Fixed: every guest VSync(n) was answered with one field
`StockVSync::serve` suspended the phase and the next step committed one field whatever n, and a phase turn got a whole
field of budget after the CD IRQ and libds work of the field start (about 13 600 ticks) had already run, so 1776 of the
first 6000 steps ended past the field edge and the boundary drifted off the field grid from step 747. Now `StockVSync`
notes the wait (`Timing::noteVSyncWait`), the driver commits `Timing::takeStepFields()` with one SPU advance per field,
and the phase turn (`ResumableContinuation::advance`) takes `ExecutionBudget::untilFieldEdge`, ending on the next field
edge (psxport `docs/presentation.md`, Frame fence and pacing).

Shown unpaced with `PSXPORT_DEBUG=fielddigest,presentseq`:

| run | | cadence (fields/parts) | steps off the field grid | present shots |
|---|---|---|---|---|
| boot, 6000 steps | before | 6000 × 1/1 | 1776 (from 747) | 1000 `62be6f42`, 2000 `7d6b22b9`, 4000 `d19581d5`, 6000 `953e01c4` |
| | after | 5828 × 1/1, 163 × 2/1, 9 × 3/1 | 0 | 1000 `72e61d56`, 2000 `46c397da`, 4000 `36a5ad9d`, 6000 identical |
| New Game to BATTLE (taps at 8600/12000/12600, `warp 0` at 14000), 18000 steps | before | 18000 × 1/1 | 3554 | 16000 and 18000 `b193d4bd` |
| | after | 14079 × 1/1, 3638 × 2/1, 263 × 4/1, 19 × 3/1, one 10/1 | 0 | identical |

Guest RAM hashes match through step 746 and differ from 747, the first turn that is now cut at the field edge; the
boot shots at 1000 to 4000 differ for the same reason (the intro reaches a given frame at another step).

## Left
- The phases' update and draw still run as guest code inside the overlays; the driver owns the cadence at the replaced
  wait, not the per-frame calls.
- No console-oracle comparison of the cadence; which n BATTLE passes per scene is not tabulated.
