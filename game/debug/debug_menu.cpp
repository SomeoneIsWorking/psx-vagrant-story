#include "debug/debug_menu.h"

#include "core.h"
#include "frame/frame_driver.h"
#include "frame/location_warp.h"
#include "game.h"
#include "runtime/session.h"

#include <lucent/log.h>

#include <vector>

namespace vagrant {

DebugMenu::DebugMenu() {
  warp({
      .levels = {"save point"},
      .places =
          [](Core &) {
            std::vector<psx::game::WarpPlace> points;
            points.reserve(LocationWarp::kSavePoints.size());
            for (std::size_t point = 0; point < LocationWarp::kSavePoints.size(); ++point) {
              points.push_back({
                  .name = lucent::format("save point {}", point),
                  .arrive =
                      [point](Core &core) {
                        return core.game->frameDriverAs<FrameDriver>().armWarp(static_cast<std::uint32_t>(point));
                      },
              });
            }
            return points;
          },
      .refusal =
          [](Core &core) {
            return core.game->frameDriverIf<FrameDriver>() != nullptr ? std::string() : "the game loop is not running";
          },
  });
  verb({
      .name = "start",
      .summary = "Start a new game at a save point, armed before the title menu opens",
      .params = {psx::game::DebugParam::named("save point",
                                              {.count =
                                                   [](Core &) {
                                                     return static_cast<int>(LocationWarp::kSavePoints.size());
                                                   },
                                               .name =
                                                   [](Core &, int point) {
                                                     return lucent::format("save point {}", point);
                                                   }})},
      .run =
          [](const psx::game::DebugCall &call) {
            call.reply = session(call.core).savePointStart.arm(call.values.u32(0));
          },
      .refusal =
          [](Core &core) {
            return session(core).titleExec.state() == TitleExecState::Cold ? std::string()
                                                                           : "the title menu has already opened";
          },
  });
}

} // namespace vagrant
