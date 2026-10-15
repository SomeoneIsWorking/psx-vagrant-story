#pragma once

#include "boot/exec_title_tail.h"
#include "boot/resident_phase.h"
#include "cd/libds_field.h"
#include "game_runtime.h"
#include "images/overlay_images.h"
#include "render/battle_projection.h"
#include "render/title_splash.h"
#include "save/title_memcard_init.h"
#include "save/title_save_check.h"
#include "title/title_exec_phase.h"

#include <array>

class Core;

namespace vagrant {

// What one Core of Vagrant Story owns outside guest RAM, created with its Game.
class Session final : public psx::game::TitleSessionState {
public:
  explicit Session(Core &core);
  // The overlay table a test stages in place of the disc's.
  Session(Core &core, std::array<OverlaySpec, 3> specs);

  OverlayImages overlayImages;
  cd::LibDsField libDsField{};
  ResidentPhase residentPhase{};
  // The title re-authors its viewport every field, so nothing is remembered.
  BattleProjectionOwner battleProjection{};
  TitleSplashPhase titleSplash{};
  TitleMemcardInit titleMemcardInit{};
  TitleSaveCheck titleSaveCheck{};
  TitleExecPhase titleExec{};
  ExecTitleTail execTitleTail{};
};

// The session of the Game `core` belongs to; aborts when it has none.
Session &session(Core &core);

} // namespace vagrant
