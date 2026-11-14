#pragma once

#include "frame/draw_section.h"
#include "frame/shot_cuts.h"
#include "motion_set.h"
#include "replay_frame.h"

#include <array>

class Core;

namespace vagrant {

// BATTLE's in-betweens: the draw tail replayed with the camera blended, held across the logic's camera cuts.
class DrawReplay {
public:
  DrawReplay();

  [[nodiscard]] psx::present::ReplayFrame &frame() {
    return frame_;
  }
  [[nodiscard]] ShotCuts &cuts() {
    return cuts_;
  }
  // Installs the tail's entry, its texture animation step and the camera cut sites on the BATTLE image just loaded.
  [[nodiscard]] static bool install(Core &core);

private:
  std::array<psx::present::MotionTrack, 1> tracks_;
  DrawSection section_;
  psx::present::ReplayFrame frame_;
  ShotCuts cuts_;
};

} // namespace vagrant
