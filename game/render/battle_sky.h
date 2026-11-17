// BATTLE's room backdrop behind the world, drawn across the record canvas's margins.
#pragma once

class Core;

namespace vagrant::battle_sky {

// Replaces the sky dome and wraps the backdrop on the BATTLE generation; false when either refuses.
bool installBattleSky(Core &core);

} // namespace vagrant::battle_sky
