#include "VoiceBlank.h"
#include <cassert>

namespace dgk {
VoiceBlank::VoiceBlank(int tick) : m_tick{tick} {}

bool VoiceBlank::IsChord() const { return false; }

std::vector<int> VoiceBlank::GetPitches() const { return {}; }

int VoiceBlank::GetTickWithRepeats() const { return m_tick; }

int VoiceBlank::GetTickWithoutRepeats() const {
  // If there's a use case, this should be implemented, but at the moment there
  // should be none.
  assert(false);
  return m_tick;
}

void VoiceBlank::SetHighlight(bool) {}

void VoiceBlank::ScrollToYou() const {}
} // namespace dgk