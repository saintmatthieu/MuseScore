#pragma once

#include "OrchestrionTypes.h"
#include "internal/VoiceSequencer.h"
#include <array>
#include <memory>
#include <vector>

namespace dgk {
class OrchestrionSequencer {
public:
  OrchestrionSequencer(Staff rightHand, Staff leftHand);
  std::vector<NoteEvent> OnInputEvent(const NoteEvent &inputEvent);
  //! Returns noteoffs that were pending.
  std::vector<NoteEvent> GoToTick(int tick);

private:
  using Hand = std::vector<std::unique_ptr<VoiceSequencer>>;
  Hand m_rightHand;
  Hand m_leftHand;
};
} // namespace dgk
