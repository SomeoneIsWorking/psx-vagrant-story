#pragma once

#include "runtime/vagrant_runtime.h"
#include "title_catalog.h"

#include <cstddef>
#include <string_view>

namespace vagrant {

// Catalog name of this generation; provisioning names the same strings.
inline constexpr std::string_view kResidentImageName = "SLUS_010.40";

// The measured file size: PS-X header plus text.
inline constexpr std::size_t kResidentFileBytes = 0x800u + kResidentHeader.textBytes;

// SLUS_010.40 as the host authenticates it; the SHA-256 is of the file whose SHA-1 rood-reverse states.
inline constexpr psx::host::TitleIdentity kTitleIdentity{
    .displayName = "Vagrant Story",
    .serial = kResidentImageName,
    .slug = "vagrant",
    .fileSize = kResidentFileBytes,
    .sha256 = "51dfdf15a58f7de8505742b5050de2951b133ed5ce801b08bff7f60b76e5d1bf",
    .entry = kResidentHeader.entry,
    .globalPointer = kResidentHeader.globalPointer,
    .textAddress = kResidentHeader.textAddress,
    .textSize = kResidentHeader.textBytes,
    .stackAddress = kResidentHeader.stackBase,
    .stackOffset = kResidentHeader.stackOffset,
    .discEnvVar = "PSXPORT_VAGRANT_DISC",
    .discExecutablePath = kResidentImageName,
};

} // namespace vagrant
