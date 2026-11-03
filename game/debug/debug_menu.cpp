#include "debug/debug_menu.h"

#include "core.h"
#include "frame/frame_driver.h"
#include "frame/location_warp.h"
#include "game.h"

#include <lucent/log.h>

#include <vector>

namespace vagrant {

DebugMenu::DebugMenu() {
  warp({
      .levels = {"save point"},
      .places =
          [](Core &) {
            std::vector<psx::game::WarpPlace> points;
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
}

} // namespace vagrant
