#include "core.h"
#include "frame/location_warp.h"
#include "game.h"
#include "runtime/runtime.h"
#include "typed_config.h"

#include <cstdint>
#include <cstdio>
#include <memory>

namespace {

int g_calls = 0;

std::uint32_t call4(Core &, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t) {
  ++g_calls;
  return 0u;
}

bool require(bool condition, const char *message) {
  if (!condition) {
    std::fprintf(stderr, "%s\n", message);
  }
  return condition;
}

} // namespace

int main() {
  using vagrant::LocationWarp;
  // The Teleportation menu's packing: save point 0 is location 28 at (5, 3), save point 3 location 29 at (12, 12).
  const LocationWarp::Target first = LocationWarp::target(0);
  const LocationWarp::Target fourth = LocationWarp::target(3);
  if (!require(first.location == 28u && first.x == 5u && first.z == 3u, "save point 0 unpacked wrongly") ||
      !require(fourth.location == 29u && fourth.x == 12u && fourth.z == 12u, "save point 3 unpacked wrongly")) {
    return 1;
  }

  LocationWarp warp(call4);
  if (!require(warp.arm("warp").starts_with("usage:"), "a bare warp did not answer its usage") ||
      !require(warp.arm("warp 35").starts_with("refused:"), "a save point past the table was armed") ||
      !require(warp.arm("warp 3x").starts_with("refused:"), "a malformed save point was armed") ||
      !require(!warp.armed(), "a refused command left a warp armed")) {
    return 1;
  }

  vagrant::Runtime runtime;
  auto game = std::make_unique<Game>(runtime, psx::config::Config{});
  Core &core = game->core;
  core.r[29] = 0x801FFF00u;
  if (!require(!warp.apply(core, true).has_value() && g_calls == 0, "an unarmed warp called the transition") ||
      !require(warp.arm("warp 34").starts_with("ok:"), "the last save point was not armed") ||
      !require(!warp.apply(core, false).has_value() && warp.armed() && g_calls == 0,
               "a warp was taken off a BATTLE field boundary") ||
      !require(warp.apply(core, true) == 34u && !warp.armed() && g_calls == 1,
               "an armed warp at an idle BATTLE field was not taken once")) {
    return 1;
  }
  std::puts("location warp: menu packing, command parsing, field gating");
  return 0;
}
