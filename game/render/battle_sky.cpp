#include "render/battle_sky.h"

#include "core.h"
#include "execution/dynarec_dispatch.h"
#include "game.h"
#include "gte_registers.h"
#include "render/battle_cull.h"

#include <array>
#include <cstdint>

namespace vagrant::battle_sky {
namespace {

// func_8008EC48 (rood-reverse BATTLE.PRG 146C.c): the room backdrop, a tiled picture, a flat fill or the sky dome.
constexpr std::uint32_t kBackdrop = 0x8008EC48u;
constexpr std::uint32_t kTiledBackdropShown = 0x800F1DC9u; // u8 backgroundVisible
constexpr std::uint32_t kTiledBackdropLayout = 0x800F1DCCu;
constexpr std::uint32_t kFlatFill = 0x800F1DC6u; // u8: the room's five sky colours are one
// The flat fill is one POLY_FT4 over 0..320 x 0..240; its x halves sit at these offsets.
constexpr std::array<std::uint32_t, 2> kFillLeftX{0x08u, 0x18u};
constexpr std::array<std::uint32_t, 2> kFillRightX{0x10u, 0x20u};

// func_8009820C (2EA3C.s): 34 vertices through rtv0tr into a scratchpad table, then 64 gouraud triangles kept unless
// all three lie past one screen edge, linked at OT + 0x1FFC.
constexpr std::uint32_t kSkyDome = 0x8009820Cu;
constexpr std::uint32_t kVertexTable = 0x1F800100u;
constexpr std::uint32_t kVertexCount = 0x22u;
constexpr std::uint32_t kTriangleCount = 0x40u;
constexpr std::uint32_t kPacketCursor = 0x1F800000u;
constexpr std::uint32_t kOrderingTable = 0x1F800004u;
constexpr std::uint32_t kSkyBucket = 0x1FFCu;
constexpr std::uint32_t kRtv0Tr = 0x4A480012u; // the word at 0x80098234
constexpr std::int32_t kCentreX = 0xA0;
constexpr std::int32_t kCentreY = 0x78;
constexpr std::uint32_t kPolyG3Words = 0x06000000u;
constexpr std::uint32_t kPolyG3Bytes = 0x1Cu;

int canvasMargin(const Core &core) {
  return core.game->guestDisplay.plan().presentationHorizontalMargin;
}

bool allBelow(const std::array<std::int32_t, 3> &values, int limit) {
  return values[0] < limit && values[1] < limit && values[2] < limit;
}

bool allAtLeast(const std::array<std::int32_t, 3> &values, int limit) {
  return values[0] >= limit && values[1] >= limit && values[2] >= limit;
}

void skyDomeOverride(Core *core) {
  Core &c = *core;
  psx::cpu::Gte &gte = c.gte();
  std::uint32_t source = c.r[4];
  for (std::uint32_t vertex = 0; vertex < kVertexCount; ++vertex, source += 12u) {
    gte.writeData(psx::gte::kVxy0, c.mem_r32(source));
    gte.writeData(psx::gte::kVz0, c.mem_r32(source + 4u));
    const std::uint32_t colour = c.mem_r32(source + 8u);
    gte.execute(kRtv0Tr);
    const std::uint32_t row = kVertexTable + vertex * 16u;
    c.mem_w32(row + 0xCu, colour);
    c.mem_w32(row + 0x0u, gte.readData(psx::gte::kMac1));
    c.mem_w32(row + 0x4u, gte.readData(psx::gte::kMac2));
    c.mem_w32(row + 0x8u, gte.readData(psx::gte::kMac3));
  }

  const battle_cull::ScreenBox box = battle_cull::widenedBox(canvasMargin(c));
  const std::uint32_t bucket = c.mem_r32(kOrderingTable) + kSkyBucket;
  std::uint32_t link = c.mem_r32(bucket);
  std::uint32_t packet = c.mem_r32(kPacketCursor);
  for (std::uint32_t triangle = 0; triangle < kTriangleCount; ++triangle, source += 4u) {
    const std::uint32_t indices = c.mem_r32(source);
    std::array<std::uint32_t, 3> rows{};
    std::array<std::int32_t, 3> x{};
    std::array<std::int32_t, 3> y{};
    std::array<std::int32_t, 3> z{};
    for (std::uint32_t corner = 0; corner < 3u; ++corner) {
      rows[corner] = kVertexTable + ((indices >> (corner * 8u)) & 0xFFu) * 16u;
      x[corner] = static_cast<std::int32_t>(c.mem_r32(rows[corner] + 0x0u)) + kCentreX;
      y[corner] = static_cast<std::int32_t>(c.mem_r32(rows[corner] + 0x4u)) + kCentreY;
      z[corner] = static_cast<std::int32_t>(c.mem_r32(rows[corner] + 0x8u));
    }
    if (z[0] <= 0 && z[1] <= 0 && z[2] <= 0) {
      continue;
    }
    if (allBelow(x, box.left) || allAtLeast(x, box.right) || allBelow(y, box.top) || allAtLeast(y, box.bottom)) {
      continue;
    }
    for (std::uint32_t corner = 0; corner < 3u; ++corner) {
      const std::uint32_t xy =
          (static_cast<std::uint32_t>(x[corner]) & 0xFFFFu) | (static_cast<std::uint32_t>(y[corner]) << 16u);
      c.mem_w32(packet + 0x4u + corner * 8u, c.mem_r32(rows[corner] + 0xCu));
      c.mem_w32(packet + 0x8u + corner * 8u, xy);
    }
    c.mem_w32(packet, (link & 0x00FFFFFFu) | kPolyG3Words);
    link = (link & 0xFF000000u) | (packet & 0x00FFFFFFu);
    packet += kPolyG3Bytes;
  }
  c.mem_w32(bucket, link);
  c.mem_w32(kPacketCursor, packet);
}

// The flat fill's quad spans the retail 320 columns; it spans the canvas margins too.
void backdropOverride(Core *core) {
  Core &c = *core;
  const std::uint32_t packet = c.mem_r32(kPacketCursor);
  const bool tiled = c.mem_r8(kTiledBackdropShown) != 0u && c.mem_r32(kTiledBackdropLayout) != 0u;
  const bool fill = !tiled && c.mem_r8(kFlatFill) != 0u;
  dynarec::callOriginalToReturn(c, kBackdrop, "Vagrant BATTLE room backdrop");
  if (!fill) {
    return;
  }
  const battle_cull::ScreenBox box = battle_cull::widenedBox(canvasMargin(c));
  for (const std::uint32_t offset : kFillLeftX) {
    c.mem_w16(packet + offset, static_cast<std::uint16_t>(box.left));
  }
  for (const std::uint32_t offset : kFillRightX) {
    c.mem_w16(packet + offset, static_cast<std::uint16_t>(box.right));
  }
}

} // namespace

bool installBattleSky(Core &core) {
  return dynarec::installNativeOverride(core, kSkyDome, "Vagrant BATTLE sky dome", skyDomeOverride) &&
         dynarec::installNativeOverride(core, kBackdrop, "Vagrant BATTLE room backdrop", backdropOverride);
}

} // namespace vagrant::battle_sky
