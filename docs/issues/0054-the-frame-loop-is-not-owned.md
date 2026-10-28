# 0054 — The frame loop is not owned

Status: open (2026-10-28). Owner: `game/frame/frame_driver.cpp:FrameDriver::stepFrame`.

## What the code does
Each step is one field: the resident/TITLE/BATTLE phases resume until the guest's VSync `0x8001F6C4`; the per-frame wait is `vs_main_gametimeUpdate` `0x8004261C` (VSync at `0x8004262C`). C++ main is `0x8001F5EC`.

## Why it matters
The workspace rule (psxport `docs/workspace/WORKSPACE.md`, "Each title owns its frame loop") wants the frame driver to run the guest's per-frame update and draw and decide the cadence, replacing the guest's VSync waits. Until then the loop is on the fallback; it is not yet on `Timing::vsyncFields` either, since every guest VSync is answered with one field whatever its argument.

## What blocks it
VSync has 27 call sites in 19 functions; the phases' update and draw run inside overlay code that waits through `0x8004261C`. Next: own `vs_main_gametimeUpdate`'s wait as the driver's field count and drive the phase updates.
The converted driver counts `Timing::vsyncFields` of the loop's own n at the replaced wait (psxport `docs/presentation.md`, Frame fence and pacing), so an overrunning frame keeps its VBlanks, audio and fields on the guest clock's grid; it is done in that conversion, not as a separate step on the current one-field answer.
