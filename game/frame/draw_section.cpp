#include "frame/draw_section.h"

#include "core.h"
#include "frame/battle_draw_facts.h"
#include "guest_redraw.h"
#include "r3000.h"

#include <algorithm>

namespace vagrant {
namespace {

constexpr std::array<std::uint32_t, 1> kAppliedSteps{battle_draw::kTextureAnimation};

} // namespace

std::optional<std::uint32_t> DrawSection::draw(Core &core, psx::present::GuestRedraw &redraw) const {
  redraw.applied(kAppliedSteps);
  if (!redraw.resumeTo(battle_draw::kDrawTail, battle_draw::kDrawTail, battle_draw::kPresent)) {
    return std::nullopt;
  }
  const std::uint32_t head = core.r[psx::cpu::kA0];
  if (std::ranges::find(heads_, head) == heads_.end()) {
    heads_ = {heads_[1], head};
  }
  return head;
}

bool DrawSection::linked(std::uint32_t head) const {
  return head != 0u && std::ranges::find(heads_, head) != heads_.end();
}

} // namespace vagrant
