#include "boot/game_heap.h"

#include "core.h"

#include <lucent/log.h>

#include <cstddef>
#include <cstdlib>

namespace vagrant::heap {

// rood-reverse `vs_main_initHeap`.
void initHeap(Core *c) {
  const std::uint32_t node = c->r[4];  // a0: first arena block
  const std::uint32_t value = c->r[5]; // a1: arena size in bytes

  // The heapA head is a zero-capacity chain head; the arena node carries the capacity.
  c->mem_w32(kControlA + offsetof(HeapHeader, prev), node);
  c->mem_w32(kControlA + offsetof(HeapHeader, next), node);
  c->mem_w32(kControlA + offsetof(HeapHeader, blockSz), 0);

  // 16-byte units.
  HeapHeader arena{};
  arena.prev = kControlA;
  arena.next = kControlA;
  arena.blockSz = (value >> 4) - 1u;
  c->mem_w32(node + offsetof(HeapHeader, prev), arena.prev);
  c->mem_w32(node + offsetof(HeapHeader, next), arena.next);
  c->mem_w32(node + offsetof(HeapHeader, blockSz), arena.blockSz);

  // heapB starts as an empty self-linked chain.
  c->mem_w32(kControlB + offsetof(HeapHeader, prev), kControlB);
  c->mem_w32(kControlB + offsetof(HeapHeader, next), kControlB);
  c->mem_w32(kControlB + offsetof(HeapHeader, blockSz), 0);

  // Result registers callers read after setup.
  constexpr std::uint32_t kKseg0Base = 0x80050000u;
  c->r[2] = kControlB;
  c->r[3] = kKseg0Base;
}

std::uint32_t freeBytes(Core &core) {
  // Bounded walk: a free chain longer than the arena's 16-byte blocks is a corrupt list.
  constexpr std::uint32_t kMaxNodes = kArenaSize / 16u;
  std::uint32_t total = 0u;
  std::uint32_t node = core.mem_r32(kControlA + offsetof(HeapHeader, prev));
  for (std::uint32_t count = 0; node != kControlA; ++count) {
    if (count == kMaxNodes) {
      lucent::error("vagrant-heap", "free chain from 0x{:08X} does not return to its head", kControlA);
      std::abort();
    }
    total += core.mem_r32(node + offsetof(HeapHeader, blockSz)) * 16u;
    node = core.mem_r32(node + offsetof(HeapHeader, prev));
  }
  return total;
}

} // namespace vagrant::heap
