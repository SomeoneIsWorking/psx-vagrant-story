#include "cd/cd_command.h"

#include "core.h"
#include "runtime/runtime.h"
#include "runtime/session.h"

namespace vagrant::cd {

void handleCdCommand(Core *core) {
  Runtime::from(*core).libcd().command(*core);
  // CD_cw returns 0 once the command is accepted.
  if (core->r[2] == 0u) {
    session(*core).libDsField.commandSent();
  }
}

} // namespace vagrant::cd
