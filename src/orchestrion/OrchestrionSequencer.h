#pragma once

#include "OrchestrionTypes.h"
#include "internal/VoiceSequencer.h"
#include <array>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <variant>
#include <vector>

namespace dgk {
class OrchestrionSequencer {
public:
  using MidiOutCb =
      std::function<void(const std::variant<NoteEvents, PedalEvent> &)>;
  OrchestrionSequencer(int track, Staff rightHand, Staff leftHand,
                       PedalSequence, MidiOutCb);
  ~OrchestrionSequencer();
  void OnInputEvent(const NoteEvent &inputEvent);
  //! Returns noteoffs that were pending.
  void GoToTick(int tick);

  const int track;
  bool loopEnabled = false;
  int loopLeftBoundary = 0;
  std::optional<int> loopRightBoundary;

  using Hand = std::vector<std::unique_ptr<VoiceSequencer>>;

private:
  void OnInputEventRecursive(const NoteEvent &inputEvent, bool loop);
  void PostPedalEvent(PedalEvent event);

  Hand m_rightHand;
  Hand m_leftHand;
  const std::vector<const VoiceSequencer *> m_allVoices;
  const PedalSequence m_pedalSequence;
  PedalSequence::const_iterator m_pedalSequenceIt;
  const MidiOutCb m_cb;
  std::thread m_callbackThread;

  using OptTimePoint =
      std::optional<std::chrono::time_point<std::chrono::steady_clock>>;

  struct QueueEntry {
    OptTimePoint time;
    PedalEvent event;
  };
  std::queue<QueueEntry> m_callbackQueue;
  std::mutex m_callbackQueueMutex;
  std::condition_variable m_callbackQueueCv;
  bool m_stopCallbackThread = false;
  bool m_pedalDown = false;
};
} // namespace dgk
