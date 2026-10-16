---
id: 0053
title: Each frame-driver step paced and delivered two display fields, so the game ran at half speed
status: fixed
symptom: real-window run presents at 30 Hz (p50 33.5 ms), 30 audio pulls/s and 22050 of 44100 samples/s
tags: S008,frame,pacing,play-check
created: 2026-10-16
updated: 2026-10-16
---

Observed: the RESTRUCTURE §6 real-window play check (`scratch/play-check/play.py`, 16:9, visible vsynced window)
presented `intervals=600` per 20 s with p50 33.50 ms, `[audio_rate] 22049 samples/s (want 44100), 60 calls/2.0s`,
and the control-channel frame count advanced 30 per second. Expected: one field per 16.68 ms, 120 audio pulls per 2 s.

Cause: `game/frame/frame_driver.cpp:FrameDriver::stepFrame` called its own `pace` service
(`framePacer.paceSubframeFields(core, {1, 1})`) after `presentation.commit(&core, 1)`, which already waits for and
delivers that field through `GameRuntime::pacePresentation`; every step slept two field periods and advanced the
emulated display clock by two fields.

Fix: the driver's `pace` service is gone; the commit is the field's only pacing. `tests/test_frame_driver.cpp`
drives the shipping commit through `stepFrame` and requires exactly one field of emulated time per step (it
delivered two before).

After: 75 s after `warp 3` in the real window, `intervals=18513 p50=16.75ms missed=36 (0.19%) longest_miss_run=1`
over the whole run and 0 misses after the warp; `audio: reprimes=0 drops=0`; 120 audio pulls per 2 s at 44100.
The New Game route moved to field-true frames (issue 0052 lists both).
