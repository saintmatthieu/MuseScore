#include "OrchestrionSequencer.h"
#include <algorithm>
#include <iterator>
#include <numeric>

namespace dgk {
namespace {
auto MakeHand(Staff staff) {
  OrchestrionSequencer::Hand hand;
  for (auto &[voice, sequence] : staff)
    hand.emplace_back(
        std::make_unique<VoiceSequencer>(voice, std::move(sequence)));
  return hand;
}

auto MakeAllVoices(const OrchestrionSequencer::Hand &rightHand,
                   const OrchestrionSequencer::Hand &leftHand) {
  std::vector<const VoiceSequencer *> allVoices;
  allVoices.reserve(rightHand.size() + leftHand.size());
  std::transform(rightHand.begin(), rightHand.end(),
                 std::back_inserter(allVoices),
                 [](const auto &voice) { return voice.get(); });
  std::transform(leftHand.begin(), leftHand.end(),
                 std::back_inserter(allVoices),
                 [](const auto &voice) { return voice.get(); });
  return allVoices;
}
} // namespace

OrchestrionSequencer::OrchestrionSequencer(int track, Staff rightHand,
                                           Staff leftHand,
                                           PedalSequence pedalSequence,
                                           MidiOutCb cb)
    : track{track}, m_rightHand{MakeHand(std::move(rightHand))},
      m_leftHand{MakeHand(std::move(leftHand))}, m_allVoices{MakeAllVoices(
                                                     m_rightHand, m_leftHand)},
      m_pedalSequence{std::move(pedalSequence)},
      m_pedalSequenceIt{m_pedalSequence.begin()}, m_cb{std::move(cb)},
      m_callbackThread{[this] {
        while (true) {
          std::vector<QueueEntry> entries;
          {
            std::unique_lock lock{m_callbackQueueMutex};
            m_callbackQueueCv.wait(lock, [this] {
              return !m_callbackQueue.empty() || m_stopCallbackThread;
            });
            if (m_stopCallbackThread)
              return;
            while (!m_callbackQueue.empty()) {
              entries.push_back(m_callbackQueue.front());
              m_callbackQueue.pop();
            }
          }
          for (auto &entry : entries) {
            if (entry.time.has_value())
              std::this_thread::sleep_until(*entry.time);
            m_cb(std::move(entry.event));
          }
        }
      }} {}

OrchestrionSequencer::~OrchestrionSequencer() {
  if (m_pedalDown) {
    PostPedalEvent(PedalEvent{track, false});
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(100ms);
  }
  m_stopCallbackThread = true;
  m_callbackQueueCv.notify_one();
  m_callbackThread.join();
}

namespace {
auto GetNextNoteonTick(const OrchestrionSequencer::Hand &hand,
                       NoteEvent::Type type) {
  return std::accumulate(
      hand.begin(), hand.end(), std::optional<dgk::Tick>{},
      [&](const auto &acc, const auto &voice) {
        const auto tick = voice->GetNextTick(type);
        return tick.has_value()
                   ? std::make_optional(std::min(acc.value_or(*tick), *tick))
                   : acc;
      });
}
} // namespace

void OrchestrionSequencer::OnInputEvent(const NoteEvent &input) {
  OnInputEventRecursive(input, loopEnabled);
  // Make sure to release the pedal if we've reached the end of this part.
  if (m_pedalDown &&
      !GetNextNoteonTick(m_rightHand, NoteEvent::Type::noteOff) &&
      !GetNextNoteonTick(m_leftHand, NoteEvent::Type::noteOff))
    PostPedalEvent(PedalEvent{track, false});
}

namespace {
void Append(dgk::NoteEvents &output, const std::vector<int> &notes, int voice,
            float velocity, NoteEvent::Type type) {
  std::transform(notes.begin(), notes.end(), std::back_inserter(output),
                 [&](int note) {
                   return NoteEvent{type, voice, note, velocity};
                 });
}
} // namespace

void OrchestrionSequencer::OnInputEventRecursive(const NoteEvent &input,
                                                 bool loop) {

  auto &hand =
      input.pitch < 60 && !m_leftHand.empty() ? m_leftHand : m_rightHand;
  const auto nextNoteonTick = GetNextNoteonTick(hand, input.type);
  if (!nextNoteonTick.has_value() && !loop)
    return;

  if (loop && input.type == NoteEvent::Type::noteOn &&
      loopRightBoundary.has_value() &&
      (!nextNoteonTick.has_value() ||
       nextNoteonTick->withoutRepeats >= *loopRightBoundary)) {
    GoToTick(loopLeftBoundary);
    return OnInputEventRecursive(input, false);
  }

  std::vector<NoteEvent> output;
  for (auto &voiceSequencer : hand) {
    const auto next =
        voiceSequencer->OnInputEvent(input.type, input.pitch, *nextNoteonTick);
    output.reserve(output.size() + next.noteOffs.size() + next.noteOns.size());
    Append(output, next.noteOffs, voiceSequencer->voice, input.velocity,
           NoteEvent::Type::noteOff);
    Append(output, next.noteOns, voiceSequencer->voice, input.velocity,
           NoteEvent::Type::noteOn);
  };

  if (!output.empty())
    m_cb(output);

  // For the pedal we wait on the slowest of both hands.
  const auto leastPedalTick = std::accumulate(
      m_allVoices.begin(), m_allVoices.end(), std::optional<dgk::Tick>{},
      [&](const auto &acc, const VoiceSequencer *voice) {
        const auto tick = voice->GetTickForPedal();
        return tick.has_value()
                   ? std::make_optional(acc.has_value() ? std::min(*acc, *tick)
                                                        : *tick)
                   : acc;
      });

  const auto prevPedalSequenceIt = m_pedalSequenceIt;
  const auto newPedalSequenceIt =
      leastPedalTick.has_value()
          ? std::find_if(m_pedalSequenceIt, m_pedalSequence.end(),
                         [&](const auto &item) {
                           return item.tick > leastPedalTick->withRepeats;
                         })
          : m_pedalSequence.end();
  if (newPedalSequenceIt > m_pedalSequenceIt) {
    if (input.type == NoteEvent::Type::noteOff) {
      // Just a release.
      PostPedalEvent(PedalEvent{track, false});
    } else {
      const auto &item = *(newPedalSequenceIt - 1);
      PostPedalEvent(PedalEvent{track, item.down});
      m_pedalSequenceIt = newPedalSequenceIt;
    }
  }
}

void OrchestrionSequencer::GoToTick(int tick) {

  std::vector<NoteEvent> output;
  for (auto hand : {&m_rightHand, &m_leftHand}) {
    for (auto &sequencer : *hand) {
      const auto next = sequencer->GoToTick(tick);
      output.reserve(output.size() + next.size());
      std::transform(next.begin(), next.end(), std::back_inserter(output),
                     [&](int note) {
                       return NoteEvent{NoteEvent::Type::noteOff,
                                        sequencer->voice, note, 0.f};
                     });
    }
  }
  if (!output.empty())
    m_cb(output);

  m_cb(PedalEvent{track, false});
  m_pedalSequenceIt = std::lower_bound(
      m_pedalSequence.begin(), m_pedalSequence.end(), tick,
      [](const auto &item, int tick) { return item.tick < tick; });
}

void OrchestrionSequencer::PostPedalEvent(PedalEvent event) {
  OptTimePoint actionTime;
  if (event.on) {
    using namespace std::chrono_literals;
    // Delay a bit actioning the pedal so that, if it were just released, the
    // dampers have time to dampen the notes.
    actionTime = std::chrono::steady_clock::now() + 100ms;
  }

  {
    std::unique_lock lock{m_callbackQueueMutex};
    if (event.on && m_pedalDown)
      // Always insert a pedal off event between two pedal on events.
      m_callbackQueue.emplace(
          QueueEntry{std::nullopt, PedalEvent{track, false}});
    m_callbackQueue.emplace(QueueEntry{actionTime, std::move(event)});
  }

  m_callbackQueueCv.notify_one();
  m_pedalDown = event.on;
}
} // namespace dgk
