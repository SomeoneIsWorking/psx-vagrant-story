#include "boot/exec_title_tail.h"
#include "core.h"
#include "execution/guest_phase.h"
#include "frame/frame_driver.h"
#include "frame/location_warp.h"
#include "game.h"
#include "lucent/content.h"
#include "runtime/runtime.h"
#include "runtime/session.h"
#include "typed_config.h"

#include <array>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

namespace {

enum Event : std::uint8_t { Input = 1, Audio, Present, Pace, LibDs, Warp, Resume };

std::vector<int> g_events;
std::array<std::uint32_t, 5> g_warpArgs{};
std::uint32_t g_warpSpeed = 0u;

void input(Core &) {
  g_events.push_back(Input);
}
void audio(Core &) {
  g_events.push_back(Audio);
}
void present(Core &) {
  g_events.push_back(Present);
}
void pace(Core &) {
  g_events.push_back(Pace);
}
void libDs(Core &) {
  g_events.push_back(LibDs);
}
void resume(Core &) {
  g_events.push_back(Resume);
}
std::uint32_t
call4(Core &core, std::uint32_t address, std::uint32_t a0, std::uint32_t a1, std::uint32_t a2, std::uint32_t a3) {
  g_events.push_back(Warp);
  g_warpArgs = {address, a0, a1, a2, a3};
  g_warpSpeed = core.mem_r32(core.r[29] + 0x10u);
  return 0u;
}

// A vs_main_execTitle tail that stays suspended at its own VSync.
class FieldSuspension final : public vagrant::dynarec::GuestContinuation {
public:
  void begin(Core &, std::uint32_t entry, std::string_view, std::uint32_t) override {
    entry_ = entry;
  }
  vagrant::dynarec::ContinuationStep advance() override {
    return {vagrant::dynarec::ContinuationStep::Kind::Suspended, 0u, entry_, {}, true};
  }

private:
  std::uint32_t entry_ = 0u;
};

bool require(bool condition, const char *message) {
  if (!condition) {
    std::fprintf(stderr, "%s\n", message);
  }
  return condition;
}

std::string sha256(std::span<const std::uint8_t> bytes) {
  return lucent::content::sha256_hex(
      lucent::content::sha256({reinterpret_cast<const std::byte *>(bytes.data()), bytes.size()}));
}

} // namespace

int main() {
  vagrant::Runtime runtime;
  auto game = std::make_unique<Game>(runtime, psx::config::Config{});
  Core &core = game->core;
  const std::vector<std::uint8_t> battleBytes{0x08u, 0x00u, 0xE0u, 0x03u};
  const std::string digest = sha256(battleBytes);
  std::array<vagrant::OverlaySpec, 3> specs{{
      {vagrant::OverlayKind::Title, "TITLE.PRG", 0x80068800u, 4u, digest},
      {vagrant::OverlayKind::Battle, "BATTLE.PRG", 0x80068800u, 4u, digest},
      {vagrant::OverlayKind::InitBattle, "INITBTL.PRG", 0x800F9800u, 4u, digest},
  }};
  game->sessionState = std::make_unique<vagrant::Session>(core, std::move(specs));
  vagrant::Session &owners = vagrant::session(core);
  vagrant::FrameDriver driver(*game,
                              {.input = input,
                               .audio = audio,
                               .present = present,
                               .pace = pace,
                               .libDs = libDs,
                               .resumeResident = resume,
                               .call4 = call4});
  core.r[29] = 0x801FFF00u;

  driver.stepFrame(core, 41);
  if (!require(g_events == std::vector<int>{Input, Audio, Present, Pace, LibDs, Resume},
               "a field did not run input, audio, one commit, pacing, libds and the phases in order") ||
      !require(game->timing.logicFrame == 41u, "the field did not publish its host frame index")) {
    return 1;
  }

  if (!require(driver.armWarp(core, "warp 3").starts_with("ok:"), "save point 3 was not armed")) {
    return 1;
  }
  // TITLE shares BATTLE's base; a suspended TITLE field must not pass as BATTLE.
  if (!require(owners.overlayImages.load(vagrant::OverlayKind::Title, battleBytes).identity.has_value(),
               "the staged TITLE image was refused")) {
    return 1;
  }
  owners.execTitleTail = vagrant::ExecTitleTail(std::make_unique<FieldSuspension>());
  owners.execTitleTail.begin(core, 0u);
  g_events.clear();
  driver.stepFrame(core, 42);
  if (!require(g_events == std::vector<int>{Input, Audio, Present, Pace, LibDs, Resume},
               "a warp was taken while TITLE, not BATTLE, was resident")) {
    return 1;
  }

  if (!require(owners.overlayImages.load(vagrant::OverlayKind::Battle, battleBytes).identity.has_value(),
               "the staged BATTLE image was refused")) {
    return 1;
  }
  const std::uint32_t stack = core.r[29];
  core.mem_w32(vagrant::LocationWarp::kScreenTransitionStep, 1u);
  g_events.clear();
  driver.stepFrame(core, 43);
  if (!require(g_events == std::vector<int>{Input, Audio, Present, Pace, LibDs, Resume},
               "a warp was taken while BATTLE's screen transition was in flight")) {
    return 1;
  }

  core.mem_w32(vagrant::LocationWarp::kScreenTransitionStep, 0u);
  g_events.clear();
  driver.stepFrame(core, 44);
  const vagrant::LocationWarp::Target target = vagrant::LocationWarp::target(3);
  if (!require(g_events == std::vector<int>{Input, Audio, Present, Pace, LibDs, Warp, Resume},
               "the armed warp was not taken between libds and the phases") ||
      !require(g_warpArgs[0] == vagrant::LocationWarp::kLocationTransition && g_warpArgs[1] == target.location &&
                   g_warpArgs[2] == target.x && g_warpArgs[3] == target.z &&
                   g_warpArgs[4] == vagrant::LocationWarp::kTeleportEffect,
               "the warp did not call BATTLE's location transition with the menu's arguments") ||
      !require(g_warpSpeed == vagrant::LocationWarp::kTeleportSpeed, "the fifth argument was not staged at sp+0x10") ||
      !require(core.r[29] == stack, "the warp did not restore the suspended guest's stack pointer")) {
    return 1;
  }

  g_events.clear();
  driver.stepFrame(core, 45);
  if (!require(g_events == std::vector<int>{Input, Audio, Present, Pace, LibDs, Resume},
               "one armed warp was taken twice")) {
    return 1;
  }
  std::puts("frame driver: one commit per field; warp taken once at an idle BATTLE field");
  return 0;
}
