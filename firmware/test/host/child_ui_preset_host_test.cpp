// Host-side unit checks for child UI pure helpers.
#include <cassert>
#include <cstdio>
#include <cstring>
#include "child/ui/child_ui_preset.h"

int main() {
  assert(childUiPresetIsToddler("toddler"));
  assert(!childUiPresetIsToddler("standard"));
  assert(!childUiPresetIsToddler("independent"));
  assert(childUiParsePreset(nullptr) == ChildUiPreset::Toddler);
  assert(childUiParsePreset("preschool") == ChildUiPreset::Toddler);

  assert(childUiPageRelativeSlot(7, 6, 6) == 1);
  assert(childUiPageRelativeSlot(2, 0, 6) == 2);
  assert(childUiPageRelativeSlot(2, 6, 6) == -1);
  assert(childUiPageRelativeSlot(8, -1, 6) == -1);

  assert(childUiMapTaskStatus("awaiting_parent") == ChildCardVisualState::WaitingParent);
  assert(childUiMapTaskStatus("approved") == ChildCardVisualState::Complete);
  assert(childUiMapTaskStatus("locked") == ChildCardVisualState::Locked);
  assert(std::strcmp(childUiActionMark(ChildCardVisualState::Ready, false), ">") == 0);
  assert(std::strcmp(childUiActionMark(ChildCardVisualState::Ready, true), "*") == 0);

  std::puts("child_ui_preset_host_test: ok");
  return 0;
}
