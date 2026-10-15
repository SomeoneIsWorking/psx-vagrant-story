#pragma once

#include "frame/location_warp.h"
#include "game_runtime.h"

#include <cstdint>
#include <string>

class Core;
class Game;

namespace vagrant {

// One display field: input, audio, the one presentation commit, pacing, the libds field service, an armed
// warp, then the resident/TITLE/BATTLE phases resume and draw what the next field commits.
class FrameDriver final : public psx::game::FrameDriver {
public:
  // The field's work as replaceable steps, so a test drives the shipping stepFrame body.
  struct Services {
    using FieldService = void (*)(Core &);

    FieldService input = nullptr;
    FieldService audio = nullptr;
    FieldService present = nullptr;
    FieldService pace = nullptr;
    FieldService libDs = nullptr;
    FieldService resumeResident = nullptr;
    LocationWarp::Call4 call4 = nullptr;
  };

  static Services productionServices();

  explicit FrameDriver(Game &game);
  FrameDriver(Game &game, Services services);

  void stepFrame(Core &core, std::uint32_t frame) override;
  // Arms a `warp <save point>` request; a later field takes it through BATTLE's own location transition.
  std::string armWarp(Core &core, const char *line);

  static FrameDriver &from(Core &core);

private:
  static void requireServices(const Services &services);

  Game &game_;
  Services services_;
  LocationWarp warp_;
};

} // namespace vagrant
