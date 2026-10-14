#pragma once

#include "game_runtime.h"
#include "platform_hle.h"
#include "psx_exe_image.h"

#include <memory>
#include <span>

struct PlatformHlePlan;

namespace vagrant {

// Header facts from the authenticated SLUS_010.40; the framework image view holds only mapping facts.
struct ResidentHeaderFacts {
  std::uint32_t entry;
  std::uint32_t globalPointer;
  std::uint32_t textAddress;
  std::uint32_t textBytes;
  std::uint32_t stackBase;
  std::uint32_t stackOffset;
};

inline constexpr ResidentHeaderFacts kResidentHeader{
    .entry = 0x8001F544u,
    .globalPointer = 0u,
    .textAddress = 0x80010000u,
    .textBytes = 0x00052000u,
    .stackBase = 0x801FFFF0u,
    .stackOffset = 0u,
};

class VagrantRuntime final : public GameRuntime {
public:
  VagrantRuntime() = default;

  void *createContext(Core &) override;
  void destroyContext(void *context) override;
  void registerOverrides(Game &game) override;
  void bootInit(Core &core) override;
  const GuestProgramImage *guestProgramImage() const override;
  RenderCapabilities renderCapabilities() const override;
  bool guestVramIsPicture(const Game &game) const override;
  // The libetc VSync body; psxport refuses a title without one before boot.
  const PlatformHlePlan *platformHlePlan() const override;
  // Receive buffers PadInitDirect registered; without them the host never publishes a pad packet.
  const GuestPadBufferLayout *guestPadBufferLayout() const override;
  // Non-null title field driver, or the framework refuses the product loop.
  std::unique_ptr<FrameDriver> createFrameDriver(Game &game) override;

  // The player's aspect; the record canvas holds the margins and BATTLE's culls widen what is drawn.
  const GuestWidescreenProjection *guestWidescreenProjection() const override;

  // Checks the title PS-X header, publishes the resident generation and registers its native leaves against it.
  psx::cpu::PsxExeLoadResult admitExecutable(Core &core, std::span<const std::uint8_t> bytes) override;
  // The guest-leg producer census beside the framework's run-end report.
  void reportRun(Core &core) const override;

private:
  static const GuestProgramImage programImage_;
  static const PlatformHlePlan platformPlan_;
  static const GuestPadBufferLayout padLayout_;
  static const GuestWidescreenProjection widescreen_;
};

} // namespace vagrant
