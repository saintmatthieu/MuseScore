#pragma once

#include "OrchestrionTypes.h"
#include "internal/VoiceSequencer.h"
#include <array>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <random>
#include <thread>
#include <unordered_set>
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
  using OptTimePoint =
      std::optional<std::chrono::time_point<std::chrono::steady_clock>>;

  template <typename EventType> struct QueueEntry {
    OptTimePoint time;
    EventType event;
  };

  template <typename EventType> struct ThreadMembers {
    std::queue<QueueEntry<EventType>> queue;
    std::mutex mutex;
    std::condition_variable cv;
  };

  template <typename EventType>
  static std::thread MakeThread(OrchestrionSequencer &self,
                                ThreadMembers<EventType> &members,
                                std::function<void(EventType)> cb);

  void OnInputEventRecursive(const NoteEvent &inputEvent, bool loop);
  void PostPedalEvent(PedalEvent event);
  void PostNoteEvents(NoteEvents events);

  Hand m_rightHand;
  Hand m_leftHand;
  const std::vector<const VoiceSequencer *> m_allVoices;
  const PedalSequence m_pedalSequence;
  PedalSequence::const_iterator m_pedalSequenceIt;
  const MidiOutCb m_cb;

  std::thread m_pedalThread;
  ThreadMembers<PedalEvent> m_pedalThreadMembers;
  std::thread m_noteThread;
  ThreadMembers<NoteEvent> m_noteThreadMembers;

  bool m_finished = false;
  bool m_pedalDown = false;
  std::mt19937 m_rng{0};
  std::uniform_int_distribution<int> m_delayDist{0, 50000};   // microseconds
  std::uniform_int_distribution<int> m_velocityDist{70, 130}; // percents

  std::unordered_set<int> m_pressedKeys;
};
} // namespace dgk
