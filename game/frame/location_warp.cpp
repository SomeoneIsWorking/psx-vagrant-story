#include "frame/location_warp.h"

#include "core.h"

#include <lucent/log.h>

namespace vagrant {
namespace {

// Four argument slots and the fifth argument the callee reads at sp+0x10.
constexpr std::uint32_t kCallFrameBytes = 0x18u;
constexpr std::uint32_t kFifthArgumentOffset = 0x10u;

} // namespace

LocationWarp::Target LocationWarp::target(std::size_t index) {
  const std::uint32_t packed = kSavePoints.at(index) + 1u;
  return {.location = packed & 0x1FFu, .x = (packed >> 16) & 0x1Fu, .z = (packed >> 24) & 0x1Fu};
}

LocationWarp::LocationWarp(Call4 call) : call_(call) {
}

std::string LocationWarp::arm(const psx::debug::CommandArgs &args) {
  if (args.size() < 2) {
    return lucent::format("usage: warp <save point 0..{}>", kSavePoints.size() - 1);
  }
  const std::optional<std::uint32_t> parsed = args.number(1);
  if (!parsed || *parsed >= kSavePoints.size()) {
    return lucent::format("refused: {} is not a save point (0..{})", args.word(1), kSavePoints.size() - 1);
  }
  const std::size_t index = *parsed;
  armed_ = index;
  return lucent::format("ok: warp armed for save point {} (location {})", index, target(index).location);
}

std::optional<std::size_t> LocationWarp::apply(Core &core, bool battleAtField) {
  if (!armed_ || !battleAtField || core.mem_r32(kScreenTransitionStep) != 0u) {
    return std::nullopt;
  }
  const std::size_t index = *armed_;
  armed_.reset();
  const Target to = target(index);
  const std::uint32_t zone = core.mem_r8(kLocationZoneMapIds + to.location * 2u);
  const std::uint32_t map = core.mem_r8(kLocationZoneMapIds + to.location * 2u + 1u);
  core.r[29] -= kCallFrameBytes;
  core.mem_w32(core.r[29] + kFifthArgumentOffset, kTeleportSpeed);
  call_(core, kLocationTransition, to.location, to.x, to.z, kTeleportEffect);
  core.r[29] += kCallFrameBytes;
  lucent::info("vagrant-warp",
               "save point {}: location {} (zone {} map {}) at ({}, {}) requested",
               index,
               to.location,
               zone,
               map,
               to.x,
               to.z);
  return index;
}

} // namespace vagrant
