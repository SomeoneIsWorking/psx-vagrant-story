#pragma once

#include <cstdint>

namespace vagrant::battle_draw {

// vs_battle_exec's draw tail opens with the camera-angle sync; its only caller is the jal at 0x8007A3E8.
inline constexpr std::uint32_t kDrawTail = 0x80069D78u;
// func_8007629C(ot + 0x2084), called at 0x8007A794, the sole BATTLE field presenter: the flip, the frame wait, then
// DrawOTag of the head in a0.
inline constexpr std::uint32_t kPresent = 0x8007629Cu;
// The texture animation step under func_8008AC78: each slot's counter, UV and CLUT steps and its MoveImage of the next
// frame's texels; the real frame's work, which no in-between repeats.
inline constexpr std::uint32_t kTextureAnimation = 0x80091B04u;
// vs_scratch.camera; the sync derives its angles' vx and vy from position and lookAt.
inline constexpr std::uint32_t kCamera = 0x1F800034u;
// func_800BC2E8(op): the event camera's command; op & 0xF selects it.
inline constexpr std::uint32_t kCameraCommand = 0x800BC2E8u;
inline constexpr std::uint32_t kCameraCommandKind = 0x0Fu;
// Command 1 copies the target into cameraPos at once, with no move.
inline constexpr std::uint32_t kCameraCommandPlace = 1u;
// func_800BBE04(on): stores D_800F4BA0, which makes vs_battle_applyCameraState follow the spherical camera instead.
inline constexpr std::uint32_t kFollowCameraSwitch = 0x800BBE04u;
inline constexpr std::uint32_t kFollowCamera = 0x800F4BA0u;

} // namespace vagrant::battle_draw
