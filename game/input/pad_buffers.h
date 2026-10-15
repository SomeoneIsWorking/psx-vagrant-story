#pragma once

#include "stock_pad_buffers.h"

class Core;

namespace vagrant {

// The PadInitDirect receive buffers, with the button bytes in the order this title's driver reads them.
class PadBuffers {
public:
  PadBuffers();

  void deliver(Core &core, const psx::input::PadDelivery &delivery) const;

private:
  psx::input::StockPadBuffers stock_;
};

} // namespace vagrant
