cmake_minimum_required(VERSION 3.21)

project(vagrant_story_port LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Python3 REQUIRED COMPONENTS Interpreter)
include(CTest)

# The framework owns image publication, invalidation and the Lightrec executor.
set(PSXPORT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/psxport" CACHE PATH
    "psxport framework source directory (default: external/psxport)")
if(NOT EXISTS "${PSXPORT_DIR}/cmake/psxport.cmake")
  message(FATAL_ERROR "PSXPORT_DIR=${PSXPORT_DIR} has no cmake/psxport.cmake — resolve the psxport checkout")
endif()
set(PSXPORT_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(PSXPORT_BUILD_SMOKE OFF CACHE BOOL "" FORCE)
add_subdirectory("${PSXPORT_DIR}" framework EXCLUDE_FROM_ALL)

# The title's owners; the product and every test link them.
add_library(vagrant_game STATIC
  game/boot/exec_title_tail.cpp
  game/boot/game_heap.cpp
  game/boot/game_time.cpp
  game/boot/guest_rect.cpp
  game/boot/resident_phase.cpp
  game/cd/cd_command.cpp
  game/cd/ds_control.cpp
  game/cd/libds_field.cpp
  game/cd/native_file.cpp
  game/execution/dynarec_dispatch.cpp
  game/execution/guest_phase.cpp
  game/execution/native_owners.cpp
  game/frame/frame_driver.cpp
  game/frame/location_warp.cpp
  game/images/battle_transfer.cpp
  game/images/overlay_images.cpp
  game/images/title_transfer.cpp
  game/input/pad_buffers.cpp
  game/render/battle_cull.cpp
  game/render/battle_projection.cpp
  game/render/title_splash.cpp
  game/runtime/runtime.cpp
  game/runtime/session.cpp
  game/save/title_memcard_init.cpp
  game/save/title_save_check.cpp
  game/title/title_exec_phase.cpp)
target_include_directories(vagrant_game PUBLIC game)
target_link_libraries(vagrant_game PUBLIC psxport)

if(BUILD_TESTING)
  add_test(NAME vagrant_cpp_ownership
    COMMAND "${Python3_EXECUTABLE}" "${PSXPORT_DIR}/tools/check_cpp_style.py" --ownership-only
            --root "${CMAKE_CURRENT_SOURCE_DIR}" --compile-commands "${CMAKE_BINARY_DIR}")
  foreach(test_name
          battle_cull battle_projection boot_route ds_control_contract dynarec_dispatch frame_driver game_heap
          image_contract location_warp overlay_images title_transfer)
    add_executable(vagrant_${test_name} tests/test_${test_name}.cpp)
    target_link_libraries(vagrant_${test_name} PRIVATE vagrant_game psxport_test_support)
    add_test(NAME vagrant_${test_name} COMMAND vagrant_${test_name})
  endforeach()
endif()

# The shipping product: the framework main over the title's catalog and psxport's dynarec-only executor.
psxport_add_product(
  vagrant_port
  CATALOG vagrant::createCatalog
  HEADER product/catalog.h
  SOURCES game/product/catalog.cpp
  LIBRARIES vagrant_game
)
