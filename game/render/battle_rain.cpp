#include "render/battle_rain.h"

#include "core.h"
#include "execution/dynarec_dispatch.h"
#include "game.h"
#include "gte_registers.h"
#include "render/battle_cull.h"

#include <cstdint>
#include <vector>

namespace vagrant::battle_rain {
namespace {

// func_8008F440 (rood-reverse BATTLE.PRG 146C.c): each falling drop moves, is projected, and is drawn as a LINE_G2 only
// when its screen x is in (0, 320); a drop it does not draw keeps byte +0xE at 2, which decides its splash, so the
// margin drops are drawn here from the pass's own inputs and never through its state.
constexpr std::uint32_t kRainPass = 0x8008F440u;
constexpr std::uint32_t kDrops = 0x800F1DD4u;     // D_800F1DD4_t *
constexpr std::uint32_t kDropCount = 0x800F1DC0u; // s16
constexpr std::uint32_t kFall = 0x800F1D98u;      // s16[3]: the per-step move and the streak's length
constexpr std::uint32_t kFrozen = 0x800F1BA4u;    // u16
constexpr std::uint32_t kTickSpeed = 0x8005E24Cu; // u8 vs_gametime_tickspeed
constexpr std::uint32_t kDropBytes = 0x10u;
constexpr std::uint32_t kDropLanded = 0x0Cu;
constexpr std::uint32_t kDropStepShift = 0x0Eu;

constexpr std::uint32_t kPacketCursor = 0x1F800000u;
constexpr std::uint32_t kOrderingTable = 0x1F800004u;
constexpr std::uint32_t kRtps = 0x4A180001u; // RotTransPers 0x80041814: RTPS, SXY2, SZ3 >> 2

// The retail bounds around the visible test.
constexpr std::int32_t kRangeLeft = -639;
constexpr std::int32_t kRangeRight = 960;
constexpr std::int32_t kFarDepth = 2048;
constexpr std::int32_t kNearDepth = 4;
constexpr std::int32_t kScreenHeight = 240;

// setLineG2 + setSemiTrans, RGB0 (96,96,128), RGB1 (32,32,100); then insertTPage(depth, getTPage(0, 3, 0, 0)).
constexpr std::uint32_t kLineWords = 0x04000000u;
constexpr std::uint32_t kLineCodeColour0 = 0x52806060u;
constexpr std::uint32_t kLineColour1 = 0x00642020u;
constexpr std::uint32_t kLineBytes = 0x14u;
constexpr std::uint32_t kTPageWords = 0x02000000u;
constexpr std::uint32_t kTPageCommand = 0xE1000260u;
constexpr std::uint32_t kTPageBytes = 0x0Cu;

struct Drop {
  std::int16_t x = 0;
  std::int16_t y = 0;
  std::int16_t z = 0;
  std::uint8_t landed = 0;
  std::uint8_t stepShift = 0;
};

struct Projection {
  std::int32_t x = 0;
  std::int32_t y = 0;
  std::int32_t depth = 0;
  std::uint32_t xy = 0;
};

Projection project(Core &core, const Drop &drop) {
  psx::cpu::Gte &gte = core.gte();
  gte.writeData(psx::gte::kVxy0,
                static_cast<std::uint16_t>(drop.x) |
                    (static_cast<std::uint32_t>(static_cast<std::uint16_t>(drop.y)) << 16u));
  gte.writeData(psx::gte::kVz0, static_cast<std::uint32_t>(static_cast<std::int32_t>(drop.z)));
  gte.execute(kRtps);
  const std::uint32_t xy = gte.projected(2u).word();
  return {static_cast<std::int16_t>(xy & 0xFFFFu),
          static_cast<std::int16_t>(xy >> 16u),
          static_cast<std::int32_t>(gte.readData(psx::gte::kSz3) >> 2u),
          xy};
}

void link(Core &core, std::uint32_t entry, std::uint32_t packet, std::uint32_t words) {
  const std::uint32_t head = core.mem_r32(entry);
  core.mem_w32(packet, (head & 0x00FFFFFFu) | words);
  core.mem_w32(entry, (head & 0xFF000000u) | (packet & 0x00FFFFFFu));
}

void drawMarginDrop(Core &core, const Projection &head, const Projection &tail) {
  const std::uint32_t entry = core.mem_r32(kOrderingTable) + static_cast<std::uint32_t>(head.depth) * 4u;
  std::uint32_t packet = core.mem_r32(kPacketCursor);
  core.mem_w32(packet + 0x4u, kLineCodeColour0);
  core.mem_w32(packet + 0x8u, head.xy);
  core.mem_w32(packet + 0xCu, kLineColour1);
  core.mem_w32(packet + 0x10u, tail.xy);
  link(core, entry, packet, kLineWords);
  packet += kLineBytes;
  core.mem_w32(packet + 0x4u, kTPageCommand);
  core.mem_w32(packet + 0x8u, 0u);
  link(core, entry, packet, kTPageWords);
  core.mem_w32(kPacketCursor, packet + kTPageBytes);
}

void rainOverride(Core *core) {
  Core &c = *core;
  const int margin = c.game->guestDisplay.plan().presentationHorizontalMargin;
  const std::int32_t count = static_cast<std::int16_t>(c.mem_r16(kDropCount));
  const std::uint32_t drops = c.mem_r32(kDrops);
  std::vector<Drop> before;
  if (margin > 0 && drops != 0u && count > 0) {
    before.resize(static_cast<std::size_t>(count));
    for (std::int32_t index = 0; index < count; ++index) {
      const std::uint32_t drop = drops + static_cast<std::uint32_t>(index) * kDropBytes;
      before[static_cast<std::size_t>(index)] = {static_cast<std::int16_t>(c.mem_r16(drop)),
                                                 static_cast<std::int16_t>(c.mem_r16(drop + 2u)),
                                                 static_cast<std::int16_t>(c.mem_r16(drop + 4u)),
                                                 c.mem_r8(drop + kDropLanded),
                                                 c.mem_r8(drop + kDropStepShift)};
    }
  }
  const bool frozen = c.mem_r16(kFrozen) != 0u;
  const std::uint32_t steps = c.mem_r8(kTickSpeed) >> 1u;
  const auto fall = [&c](std::uint32_t axis) {
    return static_cast<std::int16_t>(c.mem_r16(kFall + axis * 2u));
  };

  dynarec::callOriginalToReturn(c, kRainPass, "Vagrant BATTLE rain");
  if (before.empty()) {
    return;
  }

  const battle_cull::ScreenBox box = battle_cull::widenedBox(margin);
  const auto gte = c.gte().saveRaw();
  for (Drop drop : before) {
    if (drop.landed != 0u) {
      continue;
    }
    for (std::uint32_t step = 0; !frozen && step < steps; ++step) {
      drop.y = static_cast<std::int16_t>(drop.y + (fall(1) << drop.stepShift));
      if (drop.stepShift == 0u) {
        drop.x = static_cast<std::int16_t>(drop.x + fall(0));
        drop.z = static_cast<std::int16_t>(drop.z + fall(2));
      }
    }
    const Projection head = project(c, drop);
    if (head.x < kRangeLeft || head.x >= kRangeRight || head.depth >= kFarDepth) {
      continue;
    }
    const bool drawable = head.y > 0 && head.y < kScreenHeight && head.depth > kNearDepth;
    const bool retailDrew = head.x > 0 && head.x < battle_cull::kRetailWidth;
    if (!drawable || retailDrew || head.x <= box.left || head.x >= box.right) {
      continue;
    }
    Drop streak = drop;
    streak.x = static_cast<std::int16_t>(drop.x - fall(0));
    streak.y = static_cast<std::int16_t>(drop.y - fall(1));
    streak.z = static_cast<std::int16_t>(drop.z - fall(2));
    const Projection tail = project(c, streak);
    drawMarginDrop(c, head, tail);
  }
  c.gte().restoreRaw(gte);
}

} // namespace

bool installBattleRain(Core &core) {
  return dynarec::installNativeOverride(core, kRainPass, "Vagrant BATTLE rain", rainOverride);
}

} // namespace vagrant::battle_rain
