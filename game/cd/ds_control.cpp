#include "cd/ds_control.h"

#include "cd/cd_facts.h"
#include "core.h"
#include "runtime/runtime.h"

#include <cstdlib>
#include <lucent/log.h>

namespace vagrant::cd {

void handleDsControl(Core &core) {
  const std::uint32_t command = core.r[4] & 0xFFu;
  const std::uint32_t param = core.r[5];
  if (!ownedControl(command)) {
    lucent::error("vagrant-cd",
                  "DsControl REFUSED command 0x{:02X} param=0x{:08X}: query/read/result semantics are "
                  "not owned",
                  command,
                  param);
    std::abort();
  }
  if ((command == 0x02u || command == 0x0Du || command == 0x0Eu) && !param) {
    lucent::error("vagrant-cd", "DsControl REFUSED command 0x{:02X}: required parameter is null", command);
    std::abort();
  }
  lucent::debug("vagrant-cd", "DsControl command 0x{:02X} param=0x{:08X}", command, param);
  Runtime::from(core).libcd().control(core);
  // DsControl blocks in retail; the native controller completes synchronously, so clear the previous retry deadline.
  core.mem_w32(kSystemState, kSystemReady);
  core.mem_w32(kCommandDeadline, 0u);
}

} // namespace vagrant::cd