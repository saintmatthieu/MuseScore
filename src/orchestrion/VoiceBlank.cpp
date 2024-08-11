#include "VoiceBlank.h"

namespace dgk {
VoiceBlank::VoiceBlank(Tick tick) : m_tick{std::move(tick)} {}

bool VoiceBlank::IsChord() const { return false; }

std::vector<int> VoiceBlank::GetPitches() const { return {}; }

Tick VoiceBlank::GetTick() const { return m_tick; }

void VoiceBlank::SetHighlight(bool) {}

void VoiceBlank::ScrollToYou() const {}
} // namespace dgk