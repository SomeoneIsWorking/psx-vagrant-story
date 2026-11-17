#include "cd/disk_loads.h"

#include "cd/cd_facts.h"
#include "cd/native_file.h"
#include "core.h"
#include "execution/dynarec_dispatch.h"

#include <cstdlib>
#include <lucent/log.h>

namespace vagrant::cd {
namespace {

std::uint32_t slotAddress(std::uint32_t index) {
  return disk::kSlots + index * disk::kSlotStride;
}

std::uint16_t slotState(Core &core, std::uint32_t index) {
  return core.mem_r16(slotAddress(index) + disk::kSlotState);
}

void addCount(Core &core, std::uint32_t offset, int delta) {
  core.mem_w16(disk::kCounts + offset, static_cast<std::uint16_t>(core.mem_r16(disk::kCounts + offset) + delta));
}

// Whole sectors, as DsGetSector delivers them; a zero size is retail's seek and moves no data.
void readSectors(Core &core, std::uint32_t lba, std::uint32_t size, std::uint32_t destination) {
  if (size == 0u) {
    return;
  }
  const SectorTransfer transfer = readNativeSectors(core, lba, size, readDiscSector);
  if (!transfer) {
    lucent::error("vagrant-cd", "disk load LBA {} ({} bytes): {}", lba, size, transfer.detail);
    std::abort();
  }
  for (std::size_t offset = 0; offset < transfer.sectors.size(); ++offset) {
    core.mem_w8(destination + static_cast<std::uint32_t>(offset), transfer.sectors[offset]);
  }
  lucent::debug("vagrant-cd", "disk load LBA {} size {} -> 0x{:08X}", lba, size, destination);
}

void readSlot(Core &core, std::uint32_t index) {
  const std::uint32_t slot = slotAddress(index);
  readSectors(core,
              core.mem_r32(slot + disk::kSlotLba),
              core.mem_r32(slot + disk::kSlotSize),
              core.mem_r32(slot + disk::kSlotDestination));
}

bool diskTakesDirectRead(Core &core) {
  return core.mem_r8(disk::kDisk) == disk::kDiskIdle && core.mem_r32(disk::kDisk + disk::kDiskBufferMode) == 0u;
}

// One retail call's pass with the disk idle: Reading becomes Loaded, the priority-0 file is read (now whole), the
// others move up. Returns whether a file was read.
bool drainPass(Core &core) {
  bool read = false;
  for (std::uint32_t index = 0; index < disk::kSlotCount; ++index) {
    const std::uint32_t slot = slotAddress(index);
    const std::uint16_t state = slotState(core, index);
    if (state == disk::kStateReading) {
      core.mem_w16(slot + disk::kSlotState, disk::kStateLoaded);
      addCount(core, 0u, -1);
      continue;
    }
    if (state != disk::kStateEnqueued) {
      continue;
    }
    const std::uint16_t priority = core.mem_r16(slot + disk::kSlotPriority);
    if (priority != 0u) {
      core.mem_w16(slot + disk::kSlotPriority, static_cast<std::uint16_t>(priority - 1u));
      continue;
    }
    readSlot(core, index);
    core.mem_w16(slot + disk::kSlotState, disk::kStateReading);
    addCount(core, 2u, -1);
    addCount(core, 0u, 1);
    read = true;
  }
  return read;
}

bool anyPending(Core &core) {
  for (std::uint32_t index = 0; index < disk::kSlotCount; ++index) {
    const std::uint16_t state = slotState(core, index);
    if (state == disk::kStateEnqueued || state == disk::kStateReading) {
      return true;
    }
  }
  return false;
}

} // namespace

void processCdQueue(Core &core) {
  if (!diskTakesDirectRead(core)) {
    dynarec::callOriginalToReturn(core, disk::kProcessCdQueue, "Vagrant _processCdQueue");
    return;
  }
  const std::uint32_t frames = disk::kDisk + disk::kDiskFramesSinceLastRead;
  core.mem_w32(frames, core.mem_r32(frames) + 1u);
  if (core.mem_r32(disk::kCounts) != 0u) {
    // Each pass moves every waiting priority up by one, so 32 slots with 16-bit priorities end within 65 536 passes.
    for (std::uint32_t pass = 0; anyPending(core); ++pass) {
      if (pass > 0x10000u) {
        lucent::error("vagrant-cd", "CD queue did not drain after {} passes", pass);
        std::abort();
      }
      if (drainPass(core)) {
        core.mem_w32(frames, 0u);
      }
    }
  }
  dynarec::callReturning0(core, disk::kDiskStateMachine);
}

void loadFile(Core &core) {
  if (!diskTakesDirectRead(core)) {
    dynarec::callOriginalToReturn(core, disk::kDiskLoadFile, "Vagrant vs_main_diskLoadFile");
    return;
  }
  const std::uint32_t lba = core.r[4];
  const std::uint32_t size = core.r[5];
  const std::uint32_t destination = core.r[6];
  const std::uint32_t returnAddress = core.r[31];
  if (dynarec::callReturning4(core, disk::kDiskInitRead, lba, size, destination, 0u) == 0u) {
    lucent::error("vagrant-cd", "_diskInitRead refused an idle disk");
    std::abort();
  }
  readSectors(core, lba, size, destination);
  // The read callback's end state: every sector counted and delivered, the disk idle.
  const std::uint32_t sectors = core.mem_r32(disk::kDisk + disk::kDiskSectorCount);
  core.mem_w32(disk::kDisk + disk::kDiskSectorsDelivered, sectors);
  core.mem_w32(disk::kDisk + disk::kDiskSectorIndex, sectors);
  core.mem_w8(disk::kDisk, disk::kDiskIdle);
  core.r[2] = 1u;
  core.r[31] = returnAddress;
}

} // namespace vagrant::cd
