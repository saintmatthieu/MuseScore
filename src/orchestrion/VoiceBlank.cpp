#include "VoiceBlank.h"

namespace dgk {
VoiceBlank::VoiceBlank(int tick) : m_tick{tick} {}

bool VoiceBlank::IsChord() const { return false; }

std::vector<int> VoiceBlank::GetPitches() const { return {}; }

int VoiceBlank::GetTick() const { return m_tick; }

void VoiceBlank::SetHighlight(bool) {}

void VoiceBlank::ScrollToYou() const {}
} // namespace dgk