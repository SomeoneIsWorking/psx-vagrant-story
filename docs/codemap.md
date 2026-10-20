# Codemap

Where code lives in the Vagrant Story port. Capability state is in `docs/project-state.md`, feature hops in
`docs/features.md`, the evidence chain in `docs/re-frontier.md`, atomic work in `docs/issues/`, the BATTLE
render plan in `docs/battle-rendering.md`. The framework side is `external/psxport/docs/codemap.md`.

Product path: `run.sh` → `tools/run.py` (provision, build) → the framework main (`psxport_add_product`, target
`vagrant_port`) → `psx::host::ProductHost` over `vagrant::createCatalog` → `TitleSession` →
`vagrant::Runtime` (`admitExecutable`, `registerOverrides`, `startProgram`) → `vagrant::FrameDriver::stepFrame`
once per display field on psxport's Lightrec executor.

## Placement rules

- Everything is in namespace `vagrant` (sub-namespaces `cd`, `dynarec`, `heap`, `resident`, `sync`, `pad`, … for
  facts). Includes name the owning directory: `"runtime/runtime.h"`; psxport headers are bare.
- `vagrant::dynarec` (`game/execution/dynarec_dispatch.*`) is the only module that spells an `ExecutionBudget`,
  resolves an image identity or chooses a dispatch form. Everything else calls guest code through it.
- `installResidentNativeOwners` decides which resident leaves are native and runs from `admitExecutable`, never
  from `registerOverrides`, which runs before any image exists.
- Per-Core title state lives on `vagrant::Session`, reached through `session(core)`; the runtime holds only what
  is the same for every Core (stock library bindings, pad layout, widescreen policy).
- A measured address lives at a name in its owner's `*_facts.h` with its provenance.
- psxport (`external/psxport`, a link to the live checkout) owns the executor, override keys, the stock
  VSync/libcd/libgte/DMA/pad classes and presentation. This repository supplies addresses and title behaviour.

## Owners

| Owner | Path | What it owns |
|---|---|---|
| `createCatalog` | `game/product/catalog.*` | The one-title catalog over `kTitleIdentity`, provisioned under `scratch/bin` |
| `vagrant::Runtime` | `game/runtime/runtime.*` | The `GameRuntime`: header check and resident publication (`admitExecutable`), `startProgram` into `ResidentPhase`, `StockVSync`/`StockLibcd`/`StockDmaCallbacks` bound at this executable's addresses, `CD_cw`, `deliverPad`, Record, the widescreen policy, the `warp` command, the run-end OT census |
| `vagrant::Session`, `session(core)` | `game/runtime/session.*` | Per-Core owners: overlay residency, libds field, resident/TITLE/save phases, the exec tail, BATTLE projection |
| `vagrant::FrameDriver` | `game/frame/frame_driver.*` | One display field: input, audio, the one `presentation.commit` (which paces and delivers the field), CD IRQ poll, libds field, an armed warp, then the phases resume |
| `LocationWarp` | `game/frame/location_warp.*` | `warp <save point>`: BATTLE's location transition `func_8007C694`, called as the Teleportation spell calls it |
| facts | `game/frame/vsync_facts.h` | libetc `VSync` and the field counter it polls |
| `ResidentPhase` | `game/boot/resident_phase.*` | The resident bootstrap and TITLE reinitialisation as host states, one per retail VSync; hands TITLE and the `vs_main_execTitle` tail to suspended guest calls |
| `ExecTitleTail` | `game/boot/exec_title_tail.*` | `vs_main_execTitle` after TITLE returns: loading screen, `_loadBattlePrg`, `vs_battle_exec`, one suspended guest call; reports whether it stopped at the guest's VSync |
| boot helpers | `game/boot/game_heap.*`, `game_time.*`, `guest_rect.*` | `vs_main_initHeap`, the game-time tick, a guest RECT |
| facts | `game/boot/resident_facts.h`, `resident_image.h` | Resident addresses and load bases; the header facts and `kTitleIdentity` |
| `cd::handleCdCommand`, `handleDsControl` | `game/cd/cd_command.*`, `ds_control.*` | `CD_cw` through the stock libcd plus the libds completion it owes; the blocking `DsControlB` body |
| `cd::LibDsField` | `game/cd/libds_field.*` | The libds field tick and the synchronous `DsInit` postcondition |
| `cd::readNativeFile` | `game/cd/native_file.*` | Whole-sector disc reads into guest RAM; a failed read leaves RAM untouched |
| `vagrant::dynarec` | `game/execution/dynarec_dispatch.*`, `guest_phase.*` | Guest calls, original calls, resumable calls across fields |
| `installResidentNativeOwners` | `game/execution/native_owners.*` | Which resident leaves are native, registered against the published generation |
| `OverlayImages` | `game/images/overlay_images.*` | TITLE/BATTLE/INITBTL residency: authenticate, publish, retire the replaced generation |
| transfers | `game/images/title_transfer.*`, `battle_transfer.*` | TITLE.PRG and BATTLE.PRG+INITBTL.PRG reads published as images |
| `PadBuffers` | `game/input/pad_buffers.*` | `StockPadBuffers` at the PadInitDirect buffers, button bytes in this driver's order |
| `TitleSplashPhase` | `game/render/title_splash.*` | The publisher/developer splash, one host field per retail VSync |
| `BattleProjectionOwner` | `game/render/battle_projection.*` | Observes the four resident SDK leaves that publish the 4:3 viewport |
| battle cull | `game/render/battle_cull.*` | BATTLE's room quad reject `0x80098014`, widened by the canvas margin |
| `TitleExecPhase` | `game/title/title_exec_phase.*` | TITLE `vs_title_exec` after the save check, as guest code; reports the option it returned |
| save phases | `game/save/title_save_check.*`, `title_memcard_init.*` | `_saveFileExists` and `_initMemcard` with finite reads in place of the CD queue |
| tools | `tools/` | Launcher, disc resolution and provisioning |

## The frame turn

| Hop | Owner |
|---|---|
| product spine | `ProductHost` → `TitleSession`: `admitExecutable` → `registerOverrides` → `startProgram` |
| field | `FrameDriver::stepFrame`: `logicFrame`, `pad.serviceFrame`, `spu_audio.frame`, `presentation.commit` (paces one field through `GameRuntime::pacePresentation`), `hle.irqPoll` on `PW_IRQ`, `LibDsField::serviceField`, warp, `TitleSplashPhase` + `ResidentPhase::advanceAfterField` |
| guest that spans fields | `GuestPhase` over a resumable call; a VSync or a spent turn suspends it, the next field resumes it |
| presentation | psxport's record path replays the guest's GP0 output; one commit per field, at 30 or 15 fps as the game chooses |

## Input, CD, widescreen, debug

| Concern | Owner |
|---|---|
| host pad | psxport `Pad::serviceFrame`, then `Runtime::deliverPad` → `PadBuffers` |
| libcd | `StockLibcd` (`CdSync` stock; `CD_cw` wrapped by `cd::handleCdCommand`); reads are synchronous |
| libds | `LibDsField` per field; `DsControlB` native |
| memory card | `TitleSaveCheck` → `TitleMemcardInit`; psxport libcard HLE delivers the card events |
| widescreen | `Runtime::guestWidescreenProjection` widens the record canvas; `0x8005E248` is never written; `battle_cull` admits the margins |
| warp | `titleCommand("warp")` → `FrameDriver::armWarp` → `LocationWarp::apply` at a BATTLE VSync field |
| run report | `Runtime::reportRun` |

## Where does it go?

| Responsibility | Owner |
|---|---|
| A measured address | the owner's `*_facts.h`, at a name, and the chain in `docs/re-frontier.md` |
| Calling or re-entering guest code | `game/execution/dynarec_dispatch.*` only |
| A new native resident leaf | `game/execution/native_owners.cpp` |
| A display field, a presentation fence or a debug route | `vagrant::FrameDriver`, and only there |
| Per-Core state | a member of `vagrant::Session` |
| BATTLE world camera, objects, interpolation | the semantic owner planned in `docs/battle-rendering.md` |
| Executor, presentation, stock library mechanics | psxport |
