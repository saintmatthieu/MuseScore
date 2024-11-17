#pragma once

#include "OrchestrionTypes.h"
#include <optional>
#include <vector>

namespace dgk {

class VoiceSequencer {
public:
  struct Next {
    std::vector<int> noteOns;
    std::vector<int> noteOffs;
  };

public:
  VoiceSequencer(int voice, std::vector<ChordPtr> chords);

  const int voice;

  Next OnInputEvent(NoteEvent::Type, int midiPitch, bool consume);
  //! Returns noteoffs that were pending.
  std::vector<int> GoToTick(int tick);

  /*!
   * If `GetNextNoteonTick().has_value()`, then `GetNextNoteoffTick() ==
   * GetNextNoteonTick()`.
   */
  std::optional<int> GetNextNoteonTick() const;
  int GetNextNoteoffTick() const;

private:
  const std::vector<ChordPtr> m_chords;
  int m_chordIndex = 0;
  std::optional<uint8_t> m_pressedKey;
  std::vector<int> m_pendingNoteoffs;
};
} // namespace dgk
