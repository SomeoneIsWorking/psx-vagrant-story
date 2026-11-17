#pragma once

#include <cstdint>

namespace vagrant::cd {

// libcd/libds leaves. CD_cw contains two waits and CD_init calls CD_sync directly; both are bound so no guest VSync
// query is reachable.
// Both blocking libds command wrappers (byte-identical bodies); TITLE's movie start calls `kDsControl`.
inline constexpr std::uint32_t kDsControl = 0x80025B7Cu;
inline constexpr std::uint32_t kDsControlB = 0x80025BE4u;
inline constexpr std::uint32_t kCdCommand = 0x80021470u;
inline constexpr std::uint32_t kCdSync = 0x80020F28u;

// _diskReset leaf and state addresses.
inline constexpr std::uint32_t kDsFlush = 0x800243A0u;
inline constexpr std::uint32_t kDiskState = 0x80055D10u;
inline constexpr std::uint32_t kDsControlBuffer = 0x80055D2Cu;
inline constexpr std::uint32_t kCdReadBuffer = 0x80050110u;

// libcd's sync-callback slot (CdSyncCallback), the interrupt-code byte its ISR sets and the 8-byte result it hands the
// callback; libds registers ds_cbsync (0x80024F34) there.
inline constexpr std::uint32_t kSyncCallbackSlot = 0x800321FCu;
inline constexpr std::uint32_t kSyncInterruptCode = 0x800324D8u;
inline constexpr std::uint32_t kSyncResult = 0x80039C50u;
inline constexpr std::uint32_t kCdlComplete = 2u;
// ds_cbready: libcd's ready callback for libds (slot 0x80032200), run by the CD interrupt for each sector.
inline constexpr std::uint32_t kDsReadyCallback = 0x8002559Cu;

// The libds system-state word and its Busy/Ready values.
inline constexpr std::uint32_t kSystemState = 0x8003269Cu;
inline constexpr std::uint32_t kCommandDeadline = 0x800326C0u;
inline constexpr std::uint32_t kSystemReady = 1u;
inline constexpr std::uint32_t kSystemBusy = 2u;
inline constexpr std::uint32_t kFieldStatusTick = 0x80024BDCu;

// Blocking libds CONTROL commands psxport's synchronous CD controller can complete; others are refused.
constexpr bool ownedControl(std::uint32_t command) {
  switch (command) {
  case 0x01: // Nop
  case 0x02: // Setloc
  case 0x07: // Standby
  case 0x08: // Stop
  case 0x09: // Pause
  case 0x0D: // Setfilter
  case 0x0E: // Setmode
  case 0x15: // SeekL
  case 0x16: // SeekP
    return true;
  default:
    return false;
  }
}

} // namespace vagrant::cd

namespace vagrant::cd::disk {

// The resident CD queue (rood-reverse `_processCdQueue`, `_cdQueue`, `_cdQueueCount`, `vs_main_disk`), offsets read
// from `_processCdQueue`'s own loads and stores.
inline constexpr std::uint32_t kProcessCdQueue = 0x80044C74u;
inline constexpr std::uint32_t kDiskStateMachine = 0x800443CCu;
// `vs_main_diskLoadFile(lba, bytes, dest)` and the `_diskInitRead` it opens with (fields, cdLoc, state 5).
inline constexpr std::uint32_t kDiskLoadFile = 0x8004493Cu;
inline constexpr std::uint32_t kDiskInitRead = 0x80044340u;
inline constexpr std::uint32_t kSlots = 0x800501E0u;
inline constexpr std::uint32_t kSlotCount = 32u;
inline constexpr std::uint32_t kSlotStride = 0x14u;
inline constexpr std::uint32_t kSlotState = 0x0u;    // s16
inline constexpr std::uint32_t kSlotPriority = 0x2u; // s16
inline constexpr std::uint32_t kSlotLba = 0x4u;
inline constexpr std::uint32_t kSlotSize = 0x8u;
inline constexpr std::uint32_t kSlotDestination = 0x10u;
inline constexpr std::uint32_t kCounts = 0x80050460u; // u16 processing, u16 queued
inline constexpr std::uint32_t kDisk = 0x80055D10u;
inline constexpr std::uint32_t kDiskBufferMode = 0x2Cu;
inline constexpr std::uint32_t kDiskSectorsDelivered = 0x04u;
inline constexpr std::uint32_t kDiskSectorCount = 0x08u;
inline constexpr std::uint32_t kDiskSectorIndex = 0x30u;
inline constexpr std::uint32_t kDiskFramesSinceLastRead = 0x44u;

inline constexpr std::uint16_t kStateReading = 2u;
inline constexpr std::uint16_t kStateEnqueued = 3u;
inline constexpr std::uint16_t kStateLoaded = 4u;
inline constexpr std::uint8_t kDiskIdle = 0u;

} // namespace vagrant::cd::disk
