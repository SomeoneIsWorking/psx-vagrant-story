#pragma once

#include "title_catalog.h"

#include <memory>

namespace vagrant {

// The product's one-title catalog, run by the framework main (psxport_add_product).
std::unique_ptr<psx::host::TitleCatalog> createCatalog();

} // namespace vagrant
