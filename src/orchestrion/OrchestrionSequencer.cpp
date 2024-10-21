#include "OrchestrionSequencer.h"
#include <algorithm>
#include <iterator>
#include <numeric>

namespace dgk {
OrchestrionSequencer::OrchestrionSequencer(Staff rightHand, Staff leftHand,
                                           MidiOutCb cb)
    : m_cb{std::move(cb)} {
  for (auto &[voice, sequence] : rightHand)
    m_rightHand.emplace_back(
        std::make_unique<VoiceSequencer>(voice, std::move(sequence)));
  for (auto &[voice, sequence] : leftHand)
    m_leftHand.emplace_back(
        std::make_unique<VoiceSequencer>(voice, std::move(sequence)));
}

void
OrchestrionSequencer::OnInputEvent(const NoteEvent &input) {

  auto &hand = input.pitch >= 60 ? m_rightHand : m_leftHand;
  const auto nextNoteonTick = std::accumulate(
      hand.begin(), hand.end(), std::optional<int>{},
      [&](const auto &acc, const auto &voice) {
        const auto tick = voice->GetNextTick(input.type);
        return tick.has_value()
                   ? std::make_optional(std::min(acc.value_or(*tick), *tick))
                   : acc;
      });

  if (!nextNoteonTick.has_value())
    return;

  std::vector<NoteEvent> output;
  for (auto &voiceSequencer : hand) {
    const auto next =
        voiceSequencer->OnInputEvent(input.type, input.pitch, *nextNoteonTick);
    output.reserve(output.size() + next.noteOffs.size() + next.noteOns.size());
    std::transform(next.noteOffs.begin(), next.noteOffs.end(),
                   std::back_inserter(output), [&](int note) {
                     return NoteEvent{NoteEvent::Type::noteOff,
                                      voiceSequencer->voice, note,
                                      input.velocity};
                   });
    std::transform(next.noteOns.begin(), next.noteOns.end(),
                   std::back_inserter(output), [&](int note) {
                     return NoteEvent{NoteEvent::Type::noteOn,
                                      voiceSequencer->voice, note,
                                      input.velocity};
                   });
  };

  if (!output.empty())
    m_cb(output);
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
}
} // namespace dgk
