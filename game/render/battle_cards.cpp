#include "render/battle_cards.h"

#include "core.h"
#include "guest_memory_map.h"

#include <cstdint>
#include <optional>

namespace vagrant::battle_cards {

namespace {

// rood-reverse BATTLE.PRG `gims` (5BF94.c): Gim[3] from vs_main_allocHeapR, null while none is loaded.
inline constexpr std::uint32_t kGims = 0x800EB9BCu;
inline constexpr std::uint32_t kGimCount = 3u;
inline constexpr std::uint32_t kGimStride = 0x3F8u;
inline constexpr std::uint32_t kGimFlags = 0x0u;      // u16: bit 0 visible, bits 3-5 drawMode, bits 6-8 loadState
inline constexpr std::uint32_t kGimBrightness = 0x3u; // u8
inline constexpr std::uint32_t kGimBackground = 0xEu; // u16 15-bit colour; func_800CBBCC tiles it around the picture
inline constexpr std::uint32_t kGimLoaded = 3u;
inline constexpr std::uint32_t kGimDrawModeFlicker = 2u; // func_800CCA90 draws mode 2 at zero brightness too

template <std::uint32_t Bytes> std::uint32_t read(const Core &core, std::uint32_t address) {
  const std::optional<std::uint32_t> offset = psx::mainRamOffset(address);
  if (!offset.has_value()) {
    return 0u;
  }
  std::uint32_t value = 0u;
  for (std::uint32_t i = 0; i < Bytes; ++i) {
    value |= static_cast<std::uint32_t>(core.ram[(*offset + i) & (psx::kMainRamBytes - 1u)]) << (8u * i);
  }
  return value;
}

} // namespace

bool opaqueCardShown(const Core &core) {
  const std::uint32_t gims = read<4>(core, kGims);
  if (gims == 0u) {
    return false;
  }
  for (std::uint32_t index = 0; index < kGimCount; ++index) {
    const std::uint32_t gim = gims + (index * kGimStride);
    const std::uint32_t flags = read<2>(core, gim + kGimFlags);
    const bool visible = (flags & 1u) != 0u;
    const std::uint32_t drawMode = (flags >> 3u) & 7u;
    const std::uint32_t loadState = (flags >> 6u) & 7u;
    const bool drawn = read<1>(core, gim + kGimBrightness) != 0u || drawMode == kGimDrawModeFlicker;
    if (visible && loadState == kGimLoaded && drawn && read<2>(core, gim + kGimBackground) != 0u) {
      return true;
    }
  }
  return false;
}

} // namespace vagrant::battle_cards
