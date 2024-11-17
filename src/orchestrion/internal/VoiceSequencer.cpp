#include "VoiceSequencer.h"
#include "IChord.h"
#include <algorithm>
#include <cassert>

namespace dgk {
VoiceSequencer::VoiceSequencer(int voice, std::vector<ChordPtr> chords)
    : voice{voice}, m_chords{std::move(chords)}, m_chordIndex{0u} {}

dgk::VoiceSequencer::Next VoiceSequencer::OnInputEvent(NoteEvent::Type event,
                                                       int midiPitch,
                                                       bool consume) {

  Finally finally{[&, chordIndex = m_chordIndex] {
    if (event == NoteEvent::Type::noteOn)
      m_pressedKey = midiPitch;
    else if (m_pressedKey == midiPitch)
      m_pressedKey.reset();
    if (chordIndex == m_chordIndex)
      return;
    if (chordIndex > 0 && chordIndex <= m_chords.size())
      m_chords[chordIndex - 1]->SetHighlight(false);
    if (chordIndex < m_chords.size())
      m_chords[chordIndex]->SetHighlight(true);
    if (m_chordIndex < m_chords.size())
      m_chords[m_chordIndex]->ScrollToYou();
  }};

  if (!consume)
    return {};

  if (event == NoteEvent::Type::noteOff && m_pressedKey.has_value() &&
      *m_pressedKey != midiPitch)
    // Probably playing with two or more fingers, legato style - ignore
    return {};

  if (m_chordIndex == m_chords.size())
    return {{}, std::move(m_pendingNoteoffs)};

  const auto &segment = *m_chords[m_chordIndex];
  const auto noteOns = segment.GetPitches();

  if (event == NoteEvent::Type::noteOff) {
    // Consume the event only if it also is a noteOff.
    if (noteOns.empty())
      ++m_chordIndex;
    return {{}, std::move(m_pendingNoteoffs)};
  }

  // Remove entries in noteOffs that are in noteOns
  m_pendingNoteoffs.erase(
      std::remove_if(m_pendingNoteoffs.begin(), m_pendingNoteoffs.end(),
                     [&noteOns](int note) {
                       return std::find(noteOns.begin(), noteOns.end(), note) !=
                              noteOns.end();
                     }),
      m_pendingNoteoffs.end());
  const auto noteOffs = m_pendingNoteoffs;
  m_pendingNoteoffs = noteOns;

  ++m_chordIndex;
  return {std::move(noteOns), std::move(noteOffs)};
}

std::vector<int> VoiceSequencer::GoToTick(int tick) {
  m_chordIndex = std::lower_bound(m_chords.begin(), m_chords.end(), tick,
                                  [](const ChordPtr &seg, int tick) {
                                    return seg->GetTick() < tick;
                                  }) -
                 m_chords.begin();
  m_pressedKey.reset();
  return std::move(m_pendingNoteoffs);
}

std::optional<int> VoiceSequencer::GetNextNoteonTick() const {
  auto index = m_chordIndex;
  while (index < m_chords.size() && m_chords[index]->GetPitches().empty())
    ++index;
  return index < m_chords.size()
             ? std::make_optional(m_chords[index]->GetTick())
             : std::nullopt;
}

int VoiceSequencer::GetNextNoteoffTick() const {
  return m_chordIndex < m_chords.size() ? m_chords[m_chordIndex]->GetTick()
                                        : m_chords.back()->GetEndTick();
}
} // namespace dgk
