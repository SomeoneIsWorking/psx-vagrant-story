// BATTLE's full-screen GIM cards (staff credits, game over): 320-wide pictures with VRAM beside them.
#pragma once

class Core;

namespace vagrant::battle_cards {

// A visible, loaded GIM with a background colour fills the 4:3 screen around its picture; nothing draws beside it.
// Only meaningful while BATTLE is the published overlay.
bool opaqueCardShown(const Core &core);

} // namespace vagrant::battle_cards
