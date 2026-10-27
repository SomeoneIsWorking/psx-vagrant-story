#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>

class Core;

namespace vagrant {

// `warp <save point>`: the Teleportation spell's own route. BATTLE's func_800735F8 calls the location
// transition func_8007C694(location, x, z, effect 0, speed 8), which starts the screen transition and the
// ZND/MPD load; the warp makes that call at a field boundary while BATTLE is idle between transitions.
class LocationWarp {
public:
  using Call4 = std::uint32_t (*)(Core &, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t);

  // MENU0.PRG `_savePointData`: location in bits 0-8, x in bits 16-20, z in bits 24-28; the menu adds 1 first.
  static constexpr std::array<std::uint32_t, 35> kSavePoints{
      0x0305001Bu, 0x0801000Eu, 0x04030199u, 0x0C0C001Cu, 0x070C0028u, 0x040200CDu, 0x0F070035u,
      0x08060093u, 0x060000CEu, 0x09080106u, 0x09080111u, 0x0C030098u, 0x090800E0u, 0x060900C8u,
      0x050800CAu, 0x050F00CCu, 0x0109009Bu, 0x0A07007Cu, 0x000400CFu, 0x080800ECu, 0x020400F1u,
      0x0705011Du, 0x06080120u, 0x04080139u, 0x04080147u, 0x0904014Fu, 0x010300A2u, 0x150C00A3u,
      0x030200D0u, 0x060600D1u, 0x1007008Bu, 0x010400ABu, 0x05020055u, 0x07060063u, 0x0A0400D2u};
  // BATTLE.PRG func_8007C694 and the step word it sets; 0 is no transition in flight.
  static constexpr std::uint32_t kLocationTransition = 0x8007C694u;
  static constexpr std::uint32_t kScreenTransitionStep = 0x800F1A48u;
  // Resident `vs_main_locationZoneMapIds`: a zone byte and a map byte per location.
  static constexpr std::uint32_t kLocationZoneMapIds = 0x8004F0CCu;
  static constexpr std::uint32_t kTeleportEffect = 0u;
  static constexpr std::uint32_t kTeleportSpeed = 8u;

  struct Target {
    std::uint32_t location = 0;
    std::uint32_t x = 0;
    std::uint32_t z = 0;
  };

  // The arguments the Teleportation menu packs for save point `index`.
  static Target target(std::size_t index);

  explicit LocationWarp(Call4 call);

  // Arms save point `index` (below kSavePoints.size()); the returned text answers the client.
  std::string arm(std::size_t index);
  bool armed() const {
    return armed_.has_value();
  }
  // Makes the armed transition when BATTLE is resident, idle and stopped at its own VSync; returns the
  // save point it took.
  std::optional<std::size_t> apply(Core &core, bool battleAtField);

private:
  Call4 call_;
  std::optional<std::size_t> armed_;
};

} // namespace vagrant
