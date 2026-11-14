#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

class Core;

namespace vagrant {

// BATTLE's models in an in-between. The draw tail's model pass steps every model's animation, movement and turn before
// func_800B07DC builds its root transform, so a replay sees the newer tick's model there: this blends it from what the
// last real pass drew at the same call, pairing by model address while its shape and bone count are unchanged.
class ActorMotion {
public:
  // The model pass starts: a real one (`t` empty) records each model's drawn motion, a replay's blends to it.
  void openPass(std::optional<float> t);
  void closePass();
  // func_800B07DC(a0 = `model`) is about to build the root transform from the model's angles, its position and the
  // root offset at 0x1F8003B0; the joint angles at +0x704 are read after it by func_800B28A8.
  void atRootTransform(Core &core, std::uint32_t model);
  // Run end: models blended and those drawn as now because the last real pass did not draw them.
  void report() const;

  // Joint rows {x, y, z, pad} of doubled 12-bit angles from +0x704; the root offset row follows at +0x84C.
  static constexpr std::size_t kMaxJoints = 41;

private:
  using Triple = std::array<std::uint16_t, 3>;
  struct Sample {
    std::uint32_t model = 0;
    std::uint32_t shape = 0;
    std::uint8_t joints = 0;
    Triple position{};
    Triple angles{};
    Triple rootOffset{};
    std::array<Triple, kMaxJoints> pose{};
  };
  [[nodiscard]] static Sample read(Core &core, std::uint32_t model);
  [[nodiscard]] static const Sample *find(const std::vector<Sample> &samples, std::uint32_t model);
  static void write(Core &core, const Sample &earlier, const Sample &later, float t);

  enum class Mode : std::uint8_t { Idle, Record, Blend };
  Mode mode_ = Mode::Idle;
  float t_ = 0.0f;
  // What the last real pass drew, and what the running one draws.
  std::vector<Sample> drawn_;
  std::vector<Sample> drawing_;
  // The models the running replay reached; a model's later calls in one pass (an attachment) follow its first.
  std::vector<std::uint32_t> blended_;
  std::uint64_t blendedCount_ = 0;
  std::uint64_t unpairedCount_ = 0;
};

} // namespace vagrant
