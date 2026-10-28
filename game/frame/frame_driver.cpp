#include "frame/frame_driver.h"

#include "core.h"
#include "execution/dynarec_dispatch.h"
#include "game.h"
#include "runtime/session.h"

#include <cstdlib>
#include <lucent/log.h>

namespace vagrant {
namespace {

void serviceAudio(Core &core) {
  core.game->spu_audio.frame();
}

void serviceInput(Core &core) {
  core.game->pad.serviceFrame();
}

void present(Core &core, std::uint32_t fields) {
  core.game->presentation.commit(&core, static_cast<int>(fields));
}

void serviceLibDs(Core &core) {
  session(core).libDsField.serviceField(core);
}

void resumePhases(Core &core) {
  Session &owners = session(core);
  owners.titleSplash.advanceAfterField(core);
  owners.residentPhase.advanceAfterField(core);
}

} // namespace

FrameDriver::Services FrameDriver::productionServices() {
  return {
      .input = serviceInput,
      .audio = serviceAudio,
      .present = present,
      .libDs = serviceLibDs,
      .resumeResident = resumePhases,
      .call4 = dynarec::callReturning4,
  };
}

FrameDriver::FrameDriver(Game &game) : FrameDriver(game, productionServices()) {
}

FrameDriver::FrameDriver(Game &game, Services services) : game_(game), services_(services), warp_(services.call4) {
  requireServices(services_);
}

void FrameDriver::requireServices(const Services &services) {
  if (services.input && services.audio && services.present && services.libDs && services.resumeResident &&
      services.call4) {
    return;
  }
  lucent::error("vagrant-frame", "the frame driver requires every field service");
  std::abort();
}

std::string FrameDriver::armWarp(std::size_t savePoint) {
  return warp_.arm(savePoint);
}

void FrameDriver::stepFrame(Core &core, std::uint32_t frame) {
  // Host frame index for diagnostics and audio; never touches the guest VBlank counter at 0x80032114.
  game_.timing.logicFrame = frame;
  game_.timing.frameTick();
  core.rsub.otAttr.beginLogicFrame(frame);

  // The fields of the last step's guest VSync(n) (vs_main_gametimeUpdate passes BATTLE's tick speed), or of a turn
  // that ran out without one.
  const std::uint32_t fields = game_.timing.takeStepFields();
  services_.input(core);
  for (std::uint32_t field = 0; field < fields; ++field) {
    services_.audio(core);
  }
  // The guest drew the last step's pass on the device; this is its one presentation.
  services_.present(core, fields);

  // Service the CD IRQ guest callback at this boundary; not a VBlank delivery.
  if ((core.pending_work & Core::PW_IRQ) != 0) {
    game_.hle.irqPoll(&core);
  }
  services_.libDs(core);

  if (warp_.armed()) {
    const Session &owners = session(core);
    const bool battleAtField =
        owners.overlayImages.resident(OverlayKind::Battle).has_value() && owners.execTitleTail.suspendedAtField();
    (void)warp_.apply(core, battleAtField);
  }

  // Resume the resident/TITLE/BATTLE phases after the field; their GPU work commits on the next field.
  services_.resumeResident(core);
}

} // namespace vagrant
