#include "ComputerKeyboardMidiController.h"
#include "OrchestrionSequencer.h"

namespace dgk {

namespace {
auto letterToMidiPitch(char letter) {
  switch (letter) {
  case 'y':
    return 58;
  case 'x':
    return 59;
  case 'k':
    return 60;
  case 'l':
    return 61;
  default:
    return 0;
  }
}
} // namespace

ComputerKeyboardMidiController::ComputerKeyboardMidiController(
    const std::function<OrchestrionSequencer &()> &getOrchestrion)
    : m_getOrchestrion{getOrchestrion} {}

void ComputerKeyboardMidiController::onAltPlusLetter(char letter) {
  if (m_pressedLetters.count(letter) > 0)
    // An auto-repeat event probably, the user keeping the finger pressed on
    // a key.
    return;

  NoteEvent evt;
  evt.type = NoteEvent::Type::noteOn;
  evt.velocity = 0.5f;
  evt.pitch = letterToMidiPitch(letter);
  if (evt.pitch == 0)
    return;
  m_pressedLetters.insert(letter);
  m_getOrchestrion().OnInputEvent(evt);
}

void ComputerKeyboardMidiController::onReleasedLetter(char letter) {
  m_pressedLetters.erase(letter);
  NoteEvent evt;
  evt.type = NoteEvent::Type::noteOff;
  evt.velocity = 0.f;
  evt.pitch = letterToMidiPitch(letter);
  if (evt.pitch != 0)
    m_getOrchestrion().OnInputEvent(evt);
}
} // namespace dgk