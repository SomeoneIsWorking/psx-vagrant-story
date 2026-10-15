#pragma once

#include "game_runtime.h"
#include "guest_widescreen_projection.h"
#include "input/pad_buffers.h"
#include "stock_dma_callbacks.h"
#include "stock_libcd.h"
#include "stock_vsync.h"

#include <cstdio>
#include <memory>
#include <span>

class Core;
class Game;

namespace vagrant {

// Vagrant Story (SLUS_010.40) on Record: the resident boot phases, the stock libetc/libcd/libapi bindings at
// this executable's addresses, the PadInitDirect buffers and the `warp` command.
class Runtime final : public psx::game::GameRuntime {
public:
  Runtime();

  // Checks the title PS-X header, publishes the resident generation and registers its native leaves against it.
  psx::cpu::PsxExeLoadResult admitExecutable(Core &core, std::span<const std::uint8_t> bytes) override;
  // The resident bootstrap is host phases, entered here instead of crt0.
  void startProgram(Core &core) override;
  void registerOverrides(Game &game) override;
  std::unique_ptr<psx::game::TitleSessionState> createSession(Game &game) override;
  std::unique_ptr<psx::game::FrameDriver> createFrameDriver(Game &game) override;
  void deliverPad(Core &core, const psx::input::PadDelivery &delivery) override;

  // The canvas holds the margins and BATTLE's culls widen what is drawn; the projection word is gameplay state.
  const GuestWidescreenProjection *guestWidescreenProjection() const override;

  // `warp <save point>`.
  bool titleCommand(Core &core, const psx::game::TitleCommandLine &input, std::FILE *reply) override;
  // The guest-leg producer census beside the framework's run-end report.
  void reportRun(Core &core) const override;

  const psx::cd::StockLibcd &libcd() const {
    return libcd_;
  }

  // The runtime of the title running on `core`; another title's core stops the run.
  static const Runtime &from(const Core &core);

private:
  psx::hle::StockVSync vsync_;
  psx::cd::StockLibcd libcd_;
  psx::hle::StockDmaCallbacks dmaCallbacks_;
  PadBuffers pad_;
  GuestWidescreenProjection widescreen_;
};

} // namespace vagrant
