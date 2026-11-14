#include "frame/draw_replay.h"

#include "core.h"
#include "execution/dynarec_dispatch.h"
#include "frame/battle_draw_facts.h"
#include "game.h"
#include "runtime/session.h"

#include <lucent/log.h>

namespace vagrant {
namespace {

using psx::present::Motion;
using psx::present::MotionField;

// vs_camera_t (rood-reverse src/include/scratch.h): position and lookAt x, y, z, then angles.vz, the roll.
constexpr std::array<MotionField, 7> kCameraFields{{
    {.offset = 0x00u, .motion = Motion::Linear},
    {.offset = 0x04u, .motion = Motion::Linear},
    {.offset = 0x08u, .motion = Motion::Linear},
    {.offset = 0x10u, .motion = Motion::Linear},
    {.offset = 0x14u, .motion = Motion::Linear},
    {.offset = 0x18u, .motion = Motion::Linear},
    {.offset = 0x28u, .motion = Motion::Angle12Low},
}};

void drawTailOverride(Core *core) {
  DrawReplay &owner = session(*core).drawReplay;
  // The replay itself enters here.
  if (!owner.frame().replaying()) {
    owner.cuts().atDrawTail();
    owner.frame().beforeDraw(*core);
  }
  dynarec::callOriginalToReturn(*core, battle_draw::kDrawTail, "Vagrant battle draw tail");
}

// A replay declares nothing: it redraws a frame the logic already ran.
void declareCut(Core &core, const char *site) {
  DrawReplay &owner = session(core).drawReplay;
  if (!owner.frame().replaying()) {
    lucent::debug("vagrant-cut", "{} placed the camera at field {}", site, core.game->timing.displayField());
    owner.cuts().declare();
  }
}

void cameraCommandOverride(Core *core) {
  const std::uint32_t kind = core->mem_r8(core->r[psx::cpu::kA0]) & battle_draw::kCameraCommandKind;
  if (kind == battle_draw::kCameraCommandPlace) {
    declareCut(*core, "the event camera command");
  }
  dynarec::callOriginalToReturn(*core, battle_draw::kCameraCommand, "Vagrant event camera command");
}

void followCameraSwitchOverride(Core *core) {
  if (core->mem_r8(battle_draw::kFollowCamera) != (core->r[psx::cpu::kA0] & 0xFFu)) {
    declareCut(*core, "the follow camera switch");
  }
  dynarec::callOriginalToReturn(*core, battle_draw::kFollowCameraSwitch, "Vagrant follow camera switch");
}

void textureAnimationOverride(Core *core) {
  dynarec::callOriginalToReturn(*core, battle_draw::kTextureAnimation, "Vagrant battle texture animation");
}

} // namespace

DrawReplay::DrawReplay() : frame_(section_, tracks_) {
  tracks_[0] = {.name = "camera", .address = battle_draw::kCamera, .fields = kCameraFields};
}

bool DrawReplay::install(Core &core) {
  return dynarec::installNativeOverride(core, battle_draw::kDrawTail, "Vagrant battle draw tail", drawTailOverride) &&
         dynarec::installNativeOverride(
             core, battle_draw::kTextureAnimation, "Vagrant battle texture animation", textureAnimationOverride) &&
         dynarec::installNativeOverride(
             core, battle_draw::kCameraCommand, "Vagrant event camera command", cameraCommandOverride) &&
         dynarec::installNativeOverride(
             core, battle_draw::kFollowCameraSwitch, "Vagrant follow camera switch", followCameraSwitchOverride);
}

} // namespace vagrant
