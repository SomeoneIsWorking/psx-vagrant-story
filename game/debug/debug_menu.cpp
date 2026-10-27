#include "debug/debug_menu.h"

#include "core.h"
#include "frame/frame_driver.h"
#include "frame/location_warp.h"
#include "game.h"

namespace vagrant {

DebugMenu::DebugMenu() {
  using psx::game::DebugCall;
  using psx::game::DebugParam;
  section("Save Point Warp");
  verb({
      .name = "warp",
      .summary = "Teleport to that save point",
      .params = {DebugParam::integer("save point", 0, LocationWarp::kSavePoints.size() - 1)},
      .run =
          [](const DebugCall &call) {
            call.reply = call.core.game->frameDriverAs<FrameDriver>().armWarp(call.values.u32(0));
          },
      .refusal =
          [](Core &core) {
            return core.game->frameDriverIf<FrameDriver>() != nullptr ? std::string() : "the game loop is not running";
          },
  });
}

} // namespace vagrant
