#pragma once

#include "IComputerKeyboardMidiController.h"
#include "IOrchestrionSequencer.h"
#include <memory>
#include <modularity/ioc.h>
#include <unordered_set>

namespace dgk
{

struct NoteEvent;
class OrchestrionSequencer;

class ComputerKeyboardMidiController : public IComputerKeyboardMidiController,
                                       public muse::Injectable
{
  muse::Inject<IOrchestrionSequencer> orchestrionSequencer = {this};

public:
  void onAltPlusLetter(char) override;
  void onReleasedLetter(char) override;

private:
  std::unordered_set<char> m_pressedLetters;
};

} // namespace dgk