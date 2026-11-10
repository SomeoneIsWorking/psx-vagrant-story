# Vagrant Story port

Ports USA `SLUS_010.40` as a title-specific native/Lightrec consumer of `external/psxport`.
Execution policy: the `dynarec-port`, `dynarec-runtime` and `dynarec-overrides` skills. Framework
facts: [`external/psxport/AGENTS.md`](external/psxport/AGENTS.md). The CC0
`external/rood-reverse` decompilation is a source aid, not an authority over the authenticated game
bytes.

## Execution and ownership

- The gameplay target runs a per-`Core` Lightrec executor over the authenticated resident executable
  and loaded `.PRG` images. It has no interpreter engine or selector; unsupported behavior fails at
  the exact guest PC. `superCall` runs the original guest body through Lightrec while suppressing
  only that override for the call.
- Overlays reuse each other's bases, not the resident's: resident text ends at `0x80062000`;
  BATTLE, TITLE and ENDING load at `0x80068800`, and other modules reuse further bases. Image
  generation is part of translation and override identity.
- psxport owns Lightrec integration, CPU/device sync, executable memory and cache mechanics; this
  repository owns image identity and title behavior.
- `vagrant::Runtime` admits the resident image, enters `ResidentPhase` from `startProgram`, binds
  psxport's stock VSync/libcd/DMA classes at this executable's addresses, and creates the per-Core
  `vagrant::Session` and `vagrant::FrameDriver`. The framework main owns the process:
  `vagrant::createCatalog` hands this title to `ProductHost`, and `TitleSession` owns the order in
  which the machine becomes a product.
- `vagrant::dynarec` (`game/execution/dynarec_dispatch.{h,cpp}`) is the only module that resolves an
  image identity, spells an `ExecutionBudget` or chooses a dispatch form.
  `vagrant::installResidentNativeOwners` decides which leaves the title owns and is reached from the
  image publication boundary, never from `registerOverrides`, which runs before any image exists.
- Native CD work preserves retail command postconditions where the synchronous host CD model cannot
  deliver the asynchronous callbacks. `NativeFile` owns authenticated extents; title memcard
  transfers keep their measured allocation, image, upload, reset and event lifecycle.
- Native BATTLE world rendering derives from named pre-GTE camera, object, material, animation and
  model state. Packet or OT replay cannot establish semantic widescreen or interpolation.

## Retail facts

- The horizontal projection word `vs_main_projectionDistance` (`0x8005E248`, setter `0x8007CCF0`) is
  gameplay state: BATTLE branches on it at `0x80074580` and `0x80074744`. Widen the canvas, never
  the word.
- The guest's 4:3 publication goes through four resident SDK leaves: `0x80041540` (`SetGeomOffset`),
  `0x80041534` (`SetGeomScreen`), `0x8002B374` (`SetDefDrawEnv`) and `0x8002B434` (`SetDefDispEnv`).
  Each is observed after it runs, never replaced.
- The title presents 2 fields per game frame (30 fps) or 4 (15 fps), chosen at run time. 60 fps is
  structurally impossible, so interpolation is a presentation decision over matching source geometry.
- The boot's display publication is 320x224 at `0x8005E18C`, set by `_initScreen` at `0x80042054`.
  BATTLE's overlay rectangle at `0x8005DFD4` is zero BSS until BATTLE.PRG loads and is not a
  cross-check target during boot.

## Running it

The workspace product slot is a single shared resource: check it before any run, never edit another
agent's claim, and never run a second product instance alongside another.

A boot field, a logo, a menu, an FMV and a clean trace are not gameplay conformance. The first
migration discriminator is 1,000/1,000 host-owned TITLE fields from the exact image with nonzero
Lightrec execution, a reached native CD override, reached resident and TITLE overrides, one scoped
`superCall`, image-generation invalidation with an unchanged-generation negative, zero guest-VSync
violations, and independent CPU/device-state comparison.

To reach a map, start a New Game (BATTLE is resident from the prologue) and send
`warp save point <0..34>` (`warp ?` lists them) on the control channel; `vagrant::LocationWarp` takes
it through BATTLE's own location transition at a field boundary. Save points reached from the New
Game prologue leave Ashley hidden on arrival (issue 0055); save point 1 faulted at 0x00000001 once and
has since loaded in two runs (issue 0052).

Disc resolution: explicit argument, `PSXPORT_VAGRANT_DISC`, `.env`, then an unambiguous
repository-root CHD. `external/psxport` resolves to the workspace's live checkout, or a clone of its
main where there is none; framework execution and HLE changes belong there.
