#pragma once

#include "execution/guest_phase.h"

#include <cstdint>
#include <memory>

class Core;

namespace vagrant {

enum class ExecTitleTailState : std::uint8_t { Cold, Running, Complete };

// `vs_main_execTitle` after `vs_title_exec` returns: loading screen, BATTLE.PRG load and `vs_battle_exec`, run as guest
// code so each retail VSync is one host field.
class ExecTitleTail {
public:
  ExecTitleTail();
  explicit ExecTitleTail(std::unique_ptr<dynarec::GuestContinuation> continuation);

  void begin(Core &core, std::uint32_t startState);
  void advanceAfterField(Core &core);

  ExecTitleTailState state() const {
    return state_;
  }
  // The guest stopped at its own VSync on the last field, not mid-frame on a spent turn.
  bool suspendedAtField() const {
    return suspendedAtField_;
  }

private:
  dynarec::GuestPhase phase_;
  ExecTitleTailState state_ = ExecTitleTailState::Cold;
  bool suspendedAtField_ = false;
};

} // namespace vagrant
