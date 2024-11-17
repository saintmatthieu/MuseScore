#include "OrchestrionSequencer.h"
#include <algorithm>
#include <iterator>
#include <numeric>

namespace dgk {
OrchestrionSequencer::OrchestrionSequencer(Staff rightHand, Staff leftHand) {
  for (auto &[voice, sequence] : rightHand)
    m_rightHand.emplace_back(
        std::make_unique<VoiceSequencer>(voice, std::move(sequence)));
  for (auto &[voice, sequence] : leftHand)
    m_leftHand.emplace_back(
        std::make_unique<VoiceSequencer>(voice, std::move(sequence)));
}

std::vector<NoteEvent>
OrchestrionSequencer::OnInputEvent(const NoteEvent &input) {
  const auto isRightHand = input.pitch >= 60;
  auto &hand = isRightHand ? m_rightHand : m_leftHand;
  const auto nextNoteonTick = std::accumulate(
      hand.begin(), hand.end(), std::optional<int>{},
      [](const auto &acc, const auto &voice) {
        const auto tick = voice->GetNextNoteonTick();
        return tick.has_value()
                   ? std::make_optional(std::min(acc.value_or(*tick), *tick))
                   : acc;
      });

  std::vector<NoteEvent> output;
  for (auto &sequencer : hand) {

    const auto noteoffTick = sequencer->GetNextNoteoffTick();
    const auto consume =
        !nextNoteonTick.has_value() || noteoffTick <= *nextNoteonTick;

    const auto next = sequencer->OnInputEvent(input.type, input.pitch, consume);
    output.reserve(output.size() + next.noteOffs.size() + next.noteOns.size());
    std::transform(next.noteOffs.begin(), next.noteOffs.end(),
                   std::back_inserter(output), [&](int note) {
                     return NoteEvent{NoteEvent::Type::noteOff,
                                      sequencer->voice, note, input.velocity};
                   });
    std::transform(next.noteOns.begin(), next.noteOns.end(),
                   std::back_inserter(output), [&](int note) {
                     return NoteEvent{NoteEvent::Type::noteOn, sequencer->voice,
                                      note, input.velocity};
                   });
  };

  const auto bothHands = {&m_rightHand, &m_leftHand};
  const auto nextNoteoffTick = std::accumulate(
      bothHands.begin(), bothHands.end(), std::numeric_limits<int>::max(),
      [](int acc, const auto &hand) {
        const auto tick = std::accumulate(
            hand->begin(), hand->end(), std::numeric_limits<int>::max(),
            [](int acc, const auto &voice) {
              const auto tick = voice->GetNextNoteoffTick();
              return std::min(acc, tick);
            });
        return std::min(acc, tick);
      });
  constexpr auto track = 0; // TODO (if necessary)
  // m_scroller->ScrollTo(nextNoteoffTick, track);
  // Use IChord::SetHighlight() and ::ScrollToYou() instead.
  return output;
}

std::vector<NoteEvent> OrchestrionSequencer::GoToTick(int tick) {

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
  return output;
}
} // namespace dgk
