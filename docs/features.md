# Features — Vagrant Story

Title hops only. Each row is where Vagrant Story joins a psxport feature hop (`psxport F-X.n`, psxport
`docs/features.md`); a feature with no row runs on the framework alone.

## F-BOOT

| hop | joins | owner |
|---|---|---|
| 1 | psxport F-BOOT.1 | `game/product/catalog.cpp:createCatalog` — `kTitleIdentity` (`game/boot/resident_image.h`) and a `vagrant::Runtime` |
| 2 | psxport F-BOOT.1 | `game/runtime/runtime.cpp:Runtime::admitExecutable` — header check, resident publication, `installResidentNativeOwners`, entry registers |
| 3 | psxport F-BOOT.5 | `game/runtime/runtime.cpp:Runtime::startProgram` — enters `ResidentPhase::begin` instead of crt0 |
| 4 | psxport F-BOOT.6 | `game/runtime/runtime.cpp:Runtime::registerOverrides` — `StockVSync` `0x8001F6C4`, `StockDmaCallbacks` `0x80032128`, `StockLibcd` (`CdSync` `0x80020F28`), `CD_cw` |
| 5 | psxport F-BOOT.6 | `game/runtime/session.cpp:Session` — per-Core overlay residency, libds field, boot/TITLE/save phases, exec tail, BATTLE projection |

## F-TURN

| hop | joins | owner |
|---|---|---|
| 1 | psxport F-TURN.2 | `game/frame/frame_driver.cpp:FrameDriver::stepFrame` — input, audio, one commit, pacing, CD IRQ poll, libds field, warp, phase resume |
| 2 | psxport F-TURN.2 | `game/boot/resident_phase.cpp:ResidentPhase::advanceAfterField` — one host state per retail VSync, then TITLE and the exec tail as suspended guest calls |
| 3 | psxport F-TURN.4 | `game/runtime/runtime.cpp:Runtime::reportRun` — the OT span census |

## F-INPUT

| hop | joins | owner |
|---|---|---|
| 1 | psxport F-INPUT.1 | `game/input/pad_buffers.cpp:PadBuffers::deliver` — `StockPadBuffers` at `0x8005DFF0`/`0x8005E012`, button bytes swapped to the driver's order |

## F-CD

| hop | joins | owner |
|---|---|---|
| 1 | psxport F-CD.1 | `game/cd/cd_command.cpp:handleCdCommand` — stock `CD_cw`, then the libds sync completion it owes |
| 2 | psxport F-CD.1 | `game/cd/ds_control.cpp:handleDsControl` — the blocking `DsControlB` body |
| 3 | psxport F-CD.3 | `game/images/overlay_images.cpp:OverlayImages` — TITLE/BATTLE/INITBTL published as image generations |

## F-RENDER

| hop | joins | owner |
|---|---|---|
| 1 | psxport F-RENDER.1 | `game/runtime/runtime.cpp:Runtime::renderCapabilities` — Record, guest VRAM is the picture |

## F-WIDE

| hop | joins | owner |
|---|---|---|
| 1 | psxport F-WIDE.1 | `game/runtime/runtime.cpp:Runtime::guestWidescreenProjection` — canvas only; `0x8005E248` is gameplay state |
| 2 | psxport F-WIDE.2 | `game/render/battle_cull.cpp:installBattleCull` — BATTLE's room quad reject `0x80098014` widened by the margin |

## VS-SAVE (title only)

| hop | owner |
|---|---|
| 1 | `game/save/title_save_check.cpp:TitleSaveCheck` — TITLE's `_saveFileExists`, one host field per retail poll |
| 2 | `game/save/title_memcard_init.cpp:TitleMemcardInit` — `_initMemcard` with finite reads in place of the libds CD queue |

## F-DEBUG

| hop | joins | owner |
|---|---|---|
| 1 | psxport F-DEBUG.1 | `game/runtime/runtime.cpp:Runtime::titleCommand` — `warp <save point 0..34>` |
| 2 | psxport F-DEBUG.2 | `game/frame/location_warp.cpp:LocationWarp::apply` — at a BATTLE VSync field with no transition in flight, calls `func_8007C694(location, x, z, 0, 8)` as the Teleportation spell does |
