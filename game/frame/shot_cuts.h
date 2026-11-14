#pragma once

namespace vagrant {

// The camera cuts BATTLE's logic declares, carried to the record that draws them. The present's wait comes before its
// DrawOTag, so the record sealed after a draw tail holds the previous tail's chain.
class ShotCuts {
public:
  // The logic placed the camera without a move.
  void declare() {
    declared_ = true;
  }
  // The draw tail began: the logic since the last one belongs to this tail's frame.
  void atDrawTail() {
    previous_ = current_;
    current_ = declared_;
    declared_ = false;
  }
  // The record being sealed holds the previous tail's chain; reported once.
  [[nodiscard]] bool takeSealed() {
    const bool cut = previous_;
    previous_ = false;
    return cut;
  }

private:
  bool declared_ = false;
  bool current_ = false;
  bool previous_ = false;
};

} // namespace vagrant
