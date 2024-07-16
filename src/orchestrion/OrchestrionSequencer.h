#pragma once

#include "OrchestrionTypes.h"
#include "internal/VoiceSequencer.h"
#include <array>
#include <functional>
#include <memory>
#include <vector>

namespace dgk {
class OrchestrionSequencer {
public:
  using MidiOutCb = std::function<void(const std::vector<NoteEvent> &events)>;
  OrchestrionSequencer(Staff rightHand, Staff leftHand, MidiOutCb);
  void OnInputEvent(const NoteEvent &inputEvent);
  //! Returns noteoffs that were pending.
  void GoToTick(int tick);

private:
  using Hand = std::vector<std::unique_ptr<VoiceSequencer>>;
  Hand m_rightHand;
  Hand m_leftHand;
  const MidiOutCb m_cb;
};
} // namespace dgk
