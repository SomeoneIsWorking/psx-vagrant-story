#include "runtime/session.h"

#include "core.h"
#include "game.h"

#include <cstdlib>
#include <lucent/log.h>
#include <utility>

namespace vagrant {

Session::Session(Core &core) : overlayImages(core) {
}

Session::Session(Core &core, std::array<OverlaySpec, 3> specs) : overlayImages(core, std::move(specs)) {
}

Session &session(Core &core) {
  auto *const owned = core.game == nullptr ? nullptr : dynamic_cast<Session *>(core.game->sessionState.get());
  if (owned == nullptr) {
    lucent::error("vagrant-core", "a Vagrant Story owner reached a Core whose Game has no Vagrant Story session");
    std::abort();
  }
  return *owned;
}

} // namespace vagrant
