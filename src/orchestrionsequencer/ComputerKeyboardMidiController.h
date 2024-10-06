#pragma once

#include <memory>
#include <unordered_set>

namespace dgk {

struct NoteEvent;
class OrchestrionSequencer;

class ComputerKeyboardMidiController {
public:
  ComputerKeyboardMidiController(const std::unique_ptr<OrchestrionSequencer> &);

  void onAltPlusLetter(char);
  void onReleasedLetter(char);

private:
  const std::unique_ptr<OrchestrionSequencer> &m_orchestrion;
  std::unordered_set<char> m_pressedLetters;
};

} // namespace dgk