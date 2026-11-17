#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

class Core;

namespace vagrant {

// A game started at a save point through TITLE's Continue exit: the new-game data `initGameData` wrote, with the
// spawn point a save made at that save point holds, and BATTLE entered as a loaded game (start state 1).
class SavePointStart {
public:
  using Call1 = std::uint32_t (*)(Core &, std::uint32_t, std::uint32_t);

  // Arms save point `index` (a LocationWarp catalogue index); the reply answers the client.
  std::string arm(std::size_t index);
  bool armed() const {
    return armed_.has_value();
  }
  // In place of the title menu: TITLE `setTitleExitFlags(continue)` and the spawn point; returns the option
  // `vs_title_exec` returns for it.
  std::uint32_t take(Core &core, Call1 call1);

private:
  std::optional<std::size_t> armed_;
};

} // namespace vagrant
