#pragma once

#include "replay_frame.h"

#include <array>
#include <cstdint>
#include <optional>

class Core;

namespace vagrant {

// vs_battle_exec's draw tail, from the camera-angle sync to the present; it adds to the OT the frame's logic began.
class DrawSection final : public psx::present::ReplaySection {
public:
  [[nodiscard]] std::optional<std::uint32_t> draw(Core &core, psx::present::GuestRedraw &redraw) const override;
  [[nodiscard]] bool linked(std::uint32_t head) const override;
  // kPresent waits for its fields before its DrawOTag.
  [[nodiscard]] bool submitsAfterWait() const override {
    return true;
  }

private:
  // The two OT heads the double buffer alternates.
  mutable std::array<std::uint32_t, 2> heads_{};
};

} // namespace vagrant
