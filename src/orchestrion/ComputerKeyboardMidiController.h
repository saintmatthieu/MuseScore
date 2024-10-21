#pragma once

#include "OrchestrionTypes.h"
#include <functional>
#include <unordered_set>

namespace dgk {

struct NoteEvent;

class ComputerKeyboardMidiController {
public:
  ComputerKeyboardMidiController(
      const std::function<OrchestrionSequencer &()> &);

  void onAltPlusLetter(char);
  void onReleasedLetter(char);

private:
  const std::function<OrchestrionSequencer &()> &m_getOrchestrion;
  std::unordered_set<char> m_pressedLetters;
};

} // namespace dgk