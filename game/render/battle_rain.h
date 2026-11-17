// BATTLE's rain, with the drops that fall in the record canvas's margins drawn too.
#pragma once

class Core;

namespace vagrant::battle_rain {

// Wraps the rain pass on the BATTLE generation; false when it refuses.
bool installBattleRain(Core &core);

} // namespace vagrant::battle_rain
