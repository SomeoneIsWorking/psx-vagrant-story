# 0056 — A state loaded at boot aborts in DsInit

Status: open (2026-11-14). Owner: `game/cd/libds_field.cpp:LibDsField` and the boot's state load.

## Observed
A BATTLE state saved over the control channel (`state save`, 20 s after `BATTLE.PRG` loaded on the New Game
route) and passed back as `PSXPORT_LOAD_STATE` aborts during boot with `synchronous DsInit returned with state 1
instead of Busy`, before any field of the loaded state runs. The same build boots and plays without the variable.

## Next
Trace whether the boot's resident phases run before the load is applied, so `LibDsField` sees the loaded libds
state under a fresh boot's expectations.
