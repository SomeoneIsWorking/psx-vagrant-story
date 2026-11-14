#include "frame/actor_motion.h"

#include "blend.h"
#include "core.h"
#include "frame/battle_draw_facts.h"

#include <lucent/log.h>

#include <algorithm>

namespace vagrant {
namespace {

using battle_draw::model::kAngles;
using battle_draw::model::kJointCount;
using battle_draw::model::kPose;
using battle_draw::model::kPosition;
using battle_draw::model::kShape;
using psx::present::lerpInt;
using psx::present::lerpTurned;

constexpr std::uint32_t kJointStride = 8u;
constexpr unsigned kAngleBits = 12u;
// func_800B28A8 halves a joint angle before masking it to 12 bits.
constexpr unsigned kJointAngleBits = 13u;

std::uint16_t lerpShort(std::uint16_t from, std::uint16_t to, float t) {
  return static_cast<std::uint16_t>(lerpInt(static_cast<std::int16_t>(from), static_cast<std::int16_t>(to), t));
}

} // namespace

void ActorMotion::openPass(std::optional<float> t) {
  blended_.clear();
  if (t) {
    mode_ = Mode::Blend;
    t_ = *t;
    return;
  }
  mode_ = Mode::Record;
  drawing_.clear();
}

void ActorMotion::closePass() {
  if (mode_ == Mode::Record) {
    drawn_.swap(drawing_);
  }
  mode_ = Mode::Idle;
}

ActorMotion::Sample ActorMotion::read(Core &core, std::uint32_t model) {
  Sample sample{.model = model, .shape = core.mem_r32(model + kShape)};
  sample.joints = static_cast<std::uint8_t>(std::min<std::size_t>(core.mem_r8(model + kJointCount), kMaxJoints));
  for (std::uint32_t axis = 0; axis < 3u; ++axis) {
    sample.position[axis] = core.mem_r16(model + kPosition + axis * 2u);
    sample.angles[axis] = core.mem_r16(model + kAngles + axis * 2u);
    sample.rootOffset[axis] = core.mem_r16(battle_draw::kRootOffset + axis * 2u);
  }
  for (std::uint32_t joint = 0; joint < sample.joints; ++joint) {
    for (std::uint32_t axis = 0; axis < 3u; ++axis) {
      sample.pose[joint][axis] = core.mem_r16(model + kPose + joint * kJointStride + axis * 2u);
    }
  }
  return sample;
}

const ActorMotion::Sample *ActorMotion::find(const std::vector<Sample> &samples, std::uint32_t model) {
  const auto it = std::find_if(samples.begin(), samples.end(), [model](const Sample &s) {
    return s.model == model;
  });
  return it == samples.end() ? nullptr : &*it;
}

void ActorMotion::write(Core &core, const Sample &earlier, const Sample &later, float t) {
  const std::uint32_t model = later.model;
  for (std::uint32_t axis = 0; axis < 3u; ++axis) {
    core.mem_w16(model + kPosition + axis * 2u, lerpShort(earlier.position[axis], later.position[axis], t));
    core.mem_w16(model + kAngles + axis * 2u,
                 static_cast<std::uint16_t>(lerpTurned(earlier.angles[axis], later.angles[axis], kAngleBits, t)));
    core.mem_w16(battle_draw::kRootOffset + axis * 2u, lerpShort(earlier.rootOffset[axis], later.rootOffset[axis], t));
  }
  for (std::uint32_t joint = 0; joint < later.joints; ++joint) {
    for (std::uint32_t axis = 0; axis < 3u; ++axis) {
      const std::uint32_t blended = lerpTurned(earlier.pose[joint][axis], later.pose[joint][axis], kJointAngleBits, t);
      core.mem_w16(model + kPose + joint * kJointStride + axis * 2u, static_cast<std::uint16_t>(blended));
    }
  }
}

void ActorMotion::atRootTransform(Core &core, std::uint32_t model) {
  if (mode_ == Mode::Record) {
    if (find(drawing_, model) == nullptr) {
      drawing_.push_back(read(core, model));
    }
    return;
  }
  if (mode_ != Mode::Blend || std::find(blended_.begin(), blended_.end(), model) != blended_.end()) {
    return;
  }
  blended_.push_back(model);
  const Sample later = read(core, model);
  const Sample *earlier = find(drawn_, model);
  if (earlier == nullptr || earlier->shape != later.shape || earlier->joints != later.joints) {
    ++unpairedCount_;
    return;
  }
  ++blendedCount_;
  write(core, *earlier, later, t_);
}

void ActorMotion::report() const {
  lucent::info("vagrant-replay",
               "battle models: {} blended in in-betweens, {} drawn as now (not drawn by the last real pass, or "
               "reshaped)",
               blendedCount_,
               unpairedCount_);
}

} // namespace vagrant
