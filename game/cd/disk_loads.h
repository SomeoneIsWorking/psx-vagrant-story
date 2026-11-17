// The resident disk's loads, finished in the call that asks for them.
#pragma once

class Core;

namespace vagrant::cd {

// `_processCdQueue` 0x80044C74: retail starts one file per frame and marks it loaded a frame after its last sector;
// this reads every queued file in the retail priority order in one call. With the disk busy (XA, a direct read) or a
// buffered disk mode it runs the retail body.
void processCdQueue(Core &core);

// `vs_main_diskLoadFile` 0x8004493C: retail polls the disk state once a frame through `vs_main_gametimeUpdate(0)` until
// the last sector lands; this opens the read with the retail `_diskInitRead` and delivers every sector at once. A busy
// disk or a buffered mode runs the retail body.
void loadFile(Core &core);

} // namespace vagrant::cd
