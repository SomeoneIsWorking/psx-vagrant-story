#include "runtime/runtime.h"

#include "boot/resident_facts.h"
#include "boot/resident_image.h"
#include "cd/cd_command.h"
#include "cd/cd_facts.h"
#include "core.h"
#include "execution/native_owners.h"
#include "frame/frame_driver.h"
#include "frame/vsync_facts.h"
#include "game.h"
#include "platform_hle.h"
#include "runtime/session.h"

#include <lucent/log.h>

#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <sstream>

namespace vagrant {
namespace {

bool matchesResidentHeader(const psx::cpu::PsxExeImage &image) {
  return image.entry == kResidentHeader.entry && image.globalPointer == kResidentHeader.globalPointer &&
         image.textAddress == kResidentHeader.textAddress && image.textBytes == kResidentHeader.textBytes &&
         image.stackBase == kResidentHeader.stackBase && image.stackOffset == kResidentHeader.stackOffset;
}

std::string headerMismatch(const psx::cpu::PsxExeImage &image) {
  std::ostringstream detail;
  detail << "SLUS_010.40 PS-X EXE header mismatch: entry=0x" << std::hex << std::uppercase << image.entry << ", text=0x"
         << image.textAddress << "+0x" << image.textBytes;
  return detail.str();
}

// CdSync is the stock body; CD_cw is the title's wrapper below, so the stock command entry stays unbound.
constexpr psx::cd::StockLibcd::Entries kLibcdEntries{.sync = cd::kCdSync};

} // namespace

Runtime::Runtime()
    : vsync_(sync::kVSync, sync::kVSyncQueryCounter), libcd_(kLibcdEntries, {}, 0u),
      dmaCallbacks_(resident::kDmaCallbackTable) {
}

const Runtime &Runtime::from(const Core &core) {
  const auto *const runtime = dynamic_cast<const Runtime *>(&core.runtime);
  if (runtime == nullptr) {
    lucent::error("vagrant-runtime", "a Vagrant Story owner ran on a core with no Vagrant Story runtime");
    std::abort();
  }
  return *runtime;
}

psx::cpu::PsxExeLoadResult Runtime::admitExecutable(Core &core, std::span<const std::uint8_t> bytes) {
  const auto parsed = psx::cpu::parsePsxExeImage(bytes);
  if (!parsed.image.has_value()) {
    return {std::nullopt, {}, parsed.detail};
  }
  if (!matchesResidentHeader(parsed.image.value())) {
    return {std::nullopt, {}, headerMismatch(parsed.image.value())};
  }
  auto loaded = psx::cpu::loadPsxExeImage(core, bytes, kResidentImageName);
  if (!loaded.identity.has_value()) {
    return loaded;
  }
  const psx::cpu::ImageIdentity residentImage = *loaded.identity;
  // The only registration site: `registerOverrides` runs before any image exists. The bytes are authenticated
  // by now, so a leaf that does not register is a defect in the owner table, not a refusable input.
  const auto registration = installResidentNativeOwners(core, residentImage);
  if (!registration) {
    lucent::error("vagrant-boot",
                  "resident generation {}/{} registered {} of {} native leaves, {} refused, first at '{}'",
                  residentImage.id,
                  residentImage.generation,
                  registration.installed,
                  registration.attempts,
                  registration.refused,
                  registration.refusedOwner);
    std::abort();
  }
  psx::cpu::applyPsxExeTopLevelRegisters(core, loaded.image);
  lucent::info(
      "vagrant-boot", "resident generation {}/{} owns its native leaves", residentImage.id, residentImage.generation);
  return loaded;
}

void Runtime::startProgram(Core &core) {
  session(core).residentPhase.begin(core);
}

void Runtime::registerOverrides(Game &game) {
  vsync_.bind(game);
  // The libapi DMA table: channel 4's slot body at 0x8001DE94 is the only clearer of the transfer flag at
  // 0x800377F0, which `_waitTransferAvailable` polls and `_initSound` parks in.
  dmaCallbacks_.bind(game);
  libcd_.bind(game);
  // libds is owed the sync callback the controller interrupt would deliver after CD_cw.
  game.platform_hle.bindStock(cd::kCdCommand, cd::handleCdCommand);
}

std::unique_ptr<psx::game::TitleSessionState> Runtime::createSession(Game &game) {
  return std::make_unique<Session>(game.core);
}

std::unique_ptr<psx::game::FrameDriver> Runtime::createFrameDriver(Game &game) {
  return std::make_unique<FrameDriver>(game);
}

void Runtime::deliverPad(Core &core, const psx::input::PadDelivery &delivery) {
  pad_.deliver(core, delivery);
}

const GuestWidescreenProjection *Runtime::guestWidescreenProjection() const {
  return &widescreen_;
}

RenderCapabilities Runtime::renderCapabilities() const {
  // The picture is the guest's GP0 output replayed from the frame record.
  return RenderCapabilities{.defaultPath = RenderPath::Record, .nativeRenderPath = false};
}

bool Runtime::guestVramIsPicture(const Game &) const {
  return true;
}

bool Runtime::titleCommand(Core &core, const psx::game::TitleCommandLine &input, std::FILE *reply) {
  if (std::strcmp(input.command, "warp") != 0) {
    return false;
  }
  std::fprintf(reply, "%s\n", FrameDriver::from(core).armWarp(core, input.line).c_str());
  return true;
}

void Runtime::reportRun(Core &core) const {
  lucent::info("vagrant-boot",
               "guest-leg producer census: {} OT span(s), {} overflow(s); zero means the guest submitted no primitive",
               core.rsub.otAttr.spanCount(),
               core.rsub.otAttr.spanOverflow());
}

} // namespace vagrant
