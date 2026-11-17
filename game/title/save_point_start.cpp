#include "title/save_point_start.h"

#include "core.h"
#include "frame/location_warp.h"
#include "title/title_exec_facts.h"

#include <cstdlib>
#include <lucent/log.h>

namespace vagrant {

std::string SavePointStart::arm(std::size_t index) {
  armed_ = index;
  return lucent::format(
      "ok: new game armed at save point {} (location {})", index, LocationWarp::target(index).location);
}

std::uint32_t SavePointStart::take(Core &core, Call1 call1) {
  if (!armed_) {
    lucent::error("vagrant-start", "no save point start is armed");
    std::abort();
  }
  const std::size_t index = *armed_;
  armed_.reset();
  const LocationWarp::Target to = LocationWarp::target(index);
  const std::uint32_t zone = core.mem_r8(LocationWarp::kLocationZoneMapIds + to.location * 2u);
  const std::uint32_t map = core.mem_r8(LocationWarp::kLocationZoneMapIds + to.location * 2u + 1u);

  (void)call1(core, title_exec::kSetTitleExitFlags, title_exec::kOptionContinue);
  core.mem_w8(title_exec::kSpawnPoint + title_exec::kSpawnZndId, static_cast<std::uint8_t>(zone));
  core.mem_w8(title_exec::kSpawnPoint + title_exec::kSpawnMpdId, static_cast<std::uint8_t>(map));
  const std::uint32_t xWord = core.mem_r16(title_exec::kSpawnPoint + title_exec::kSpawnXWord);
  core.mem_w16(title_exec::kSpawnPoint + title_exec::kSpawnXWord,
               static_cast<std::uint16_t>((xWord & ~(0x1Fu << 8)) | (to.x << 8)));
  const std::uint32_t zWord = core.mem_r16(title_exec::kSpawnPoint + title_exec::kSpawnZWord);
  core.mem_w16(title_exec::kSpawnPoint + title_exec::kSpawnZWord,
               static_cast<std::uint16_t>((zWord & ~(0x1Fu << 10)) | (to.z << 10)));
  lucent::info("vagrant-start",
               "save point {}: new game continues at location {} (zone {} map {}) ({}, {})",
               index,
               to.location,
               zone,
               map,
               to.x,
               to.z);
  return title_exec::kOptionContinue;
}

} // namespace vagrant
