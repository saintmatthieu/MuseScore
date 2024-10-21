#include "VoiceSequencer.h"
#include "IChord.h"
#include <algorithm>
#include <cassert>

namespace dgk {
VoiceSequencer::VoiceSequencer(int voice, std::vector<ChordPtr> chords)
    : voice{voice}, m_gestures{std::move(chords)},
      m_numGestures{static_cast<int>(m_gestures.size())} {}

dgk::VoiceSequencer::Next VoiceSequencer::OnInputEvent(NoteEvent::Type event,
                                                       int midiPitch,
                                                       int cursorTick) {
  const auto before = m_active;
  Advance(event, midiPitch, cursorTick);

  std::vector<int> noteOffs;
  for (auto i = before.begin; i < m_active.begin; ++i) {
    const auto &gesture = m_gestures[i];
    const auto pitches = gesture->GetPitches();
    gesture->SetHighlight(false);
    noteOffs.insert(noteOffs.end(), pitches.begin(), pitches.end());
  }

  std::vector<int> noteOns;
  for (auto i = before.end; i < m_active.end; ++i) {
    const auto pitches = m_gestures[i]->GetPitches();
    noteOns.insert(noteOns.end(), pitches.begin(), pitches.end());
  }

  // Remove entries in noteOffs that are in noteOns
  noteOffs.erase(std::remove_if(noteOffs.begin(), noteOffs.end(),
                                [&noteOns](int note) {
                                  return std::find(noteOns.begin(),
                                                   noteOns.end(),
                                                   note) != noteOns.end();
                                }),
                 noteOffs.end());

  if (m_active.begin < m_active.end) {
    m_gestures[m_active.end - 1]->SetHighlight(true);
    m_gestures[m_active.end - 1]->ScrollToYou();
  }

  return {noteOns, noteOffs};
}

void VoiceSequencer::Advance(NoteEvent::Type event, int midiPitch,
                             int cursorTick) {
  Finally finally{[&] {
    if (event == NoteEvent::Type::noteOn)
      m_pressedKey = midiPitch;
    else if (m_pressedKey == midiPitch)
      m_pressedKey.reset();
  }};

  if (event == NoteEvent::Type::noteOff && m_pressedKey.has_value() &&
      *m_pressedKey != midiPitch)
    // Probably playing with two or more fingers, legato style - ignore
    return;

  // If there is something active, then the end tick of the first active
  // gesture. Else, the tick of the next gesture.
  const auto nextBegin = GetNextBegin(event);

  if (nextBegin == m_numGestures) {
    m_active.begin = m_active.end = nextBegin;
    return;
  }

  if (cursorTick < m_gestures[nextBegin]->GetTick())
    // We're finished or the cursor hasn't reached our next event yet.
    return;

  m_active.begin = nextBegin;
  m_active.end = nextBegin + 1;
}

std::vector<int> VoiceSequencer::GoToTick(int tick) {

  std::vector<int> noteOffs;
  for (auto i = m_active.begin; i < m_active.end; ++i) {
    const auto &gesture = m_gestures[i];
    const auto pitches = gesture->GetPitches();
    gesture->SetHighlight(false);
    noteOffs.insert(noteOffs.end(), pitches.begin(), pitches.end());
  }

  m_active.begin = static_cast<int>(
      std::lower_bound(
          m_gestures.begin(), m_gestures.end(), tick,
          [](const ChordPtr &seg, int tick) { return seg->GetTick() < tick; }) -
      m_gestures.begin());
  m_active.end = m_active.begin;
  m_pressedKey.reset();
  return noteOffs;
}

int VoiceSequencer::GetNextBegin(NoteEvent::Type event) const {
  auto nextActive = m_active.end;
  while (nextActive < m_numGestures) {
    const auto isChord = !m_gestures[nextActive]->GetPitches().empty();
    if (isChord && event == NoteEvent::Type::noteOff)
      // The user is releasing a key and hence does not intend to play the next
      // chord.
      break;
    // Activate
    ++nextActive;
  }
  return nextActive;
}

std::optional<int> VoiceSequencer::GetNextTick(NoteEvent::Type event) const {
  const auto i = GetNextBegin(event);
  return i < m_numGestures ? std::make_optional(m_gestures[i]->GetTick())
                           : std::nullopt;
}
} // namespace dgk
