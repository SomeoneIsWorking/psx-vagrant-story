#include "input/pad_buffers.h"

#include "core.h"
#include "input/pad_facts.h"

namespace vagrant {
namespace {

void swapButtonBytes(Core &core, std::uint32_t buffer) {
  const std::uint8_t first = core.mem_r8(buffer + 2u);
  const std::uint8_t second = core.mem_r8(buffer + 3u);
  core.mem_w8(buffer + 2u, second);
  core.mem_w8(buffer + 3u, first);
}

} // namespace

PadBuffers::PadBuffers()
    : stock_({.slot0Buffer = pad::kSlot0Buffer,
              .slot1Buffer = pad::kSlot1Buffer,
              .slotPointerTable = pad::kDriverPointerTable,
              .slotPointerStride = pad::kDriverPointerStride}) {
}

void PadBuffers::deliver(Core &core, const psx::input::PadDelivery &delivery) const {
  stock_.deliver(core, delivery);
  const std::uint32_t fixedBuffers[] = {pad::kSlot0Buffer, pad::kSlot1Buffer};
  const std::uint32_t slots = delivery.slot1Connected ? 2u : 1u;
  for (std::uint32_t slot = 0; slot < slots; ++slot) {
    const std::uint32_t installed = core.mem_r32(pad::kDriverPointerTable + slot * pad::kDriverPointerStride);
    swapButtonBytes(core, installed != 0u ? installed : fixedBuffers[slot]);
  }
}

} // namespace vagrant
