#include "product/catalog.h"

#include "boot/resident_image.h"
#include "runtime/runtime.h"
#include "single_title_catalog.h"

namespace vagrant {

std::unique_ptr<psx::host::TitleCatalog> createCatalog() {
  return std::make_unique<psx::host::SingleTitleCatalog>(kTitleIdentity, "scratch/bin", std::make_unique<Runtime>());
}

} // namespace vagrant
