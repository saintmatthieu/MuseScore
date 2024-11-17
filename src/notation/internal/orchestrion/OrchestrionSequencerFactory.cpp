#include "OrchestrionSequencerFactory.h"
#include "MuseChord.h"
#include "Orchestrion/OrchestrionSequencer.h"
#include "engraving/dom/chord.h"
#include "engraving/dom/measure.h"
#include "engraving/dom/mscore.h"
#include "engraving/dom/note.h"
#include "engraving/dom/part.h"
#include "engraving/dom/repeatlist.h"
#include "engraving/dom/rest.h"
#include "engraving/dom/score.h"
#include "engraving/dom/staff.h"

namespace dgk {
namespace {
std::vector<const mu::engraving::Chord *>
GetChords(const mu::engraving::Segment &segment, size_t nScoreTracks) {
  std::vector<const mu::engraving::Chord *> chords;
  auto trackId = 0;
  while (trackId < nScoreTracks) {
    if (const auto chord = dynamic_cast<const mu::engraving::Chord *>(
            segment.element(trackId))) {
      const auto staff = chord->staff();
      if (staff->visible() &&
          !staff->staffType(mu::engraving::Fraction(0, 1))->isMuted())
        chords.push_back(chord);
    }
    ++trackId;
  }
  return chords;
}

bool HasUntiedNotes(const mu::engraving::Chord &chord) {
  return std::any_of(
      chord.notes().begin(), chord.notes().end(),
      [](const mu::engraving::Note *note) { return !note->tieBack(); });
}

bool TakeIt(const mu::engraving::Segment &segment, size_t staffIdx, int voice,
            bool &prevWasRest) {
  const auto track = staffIdx * mu::engraving::VOICES + voice;
  const auto element = segment.element(track);
  if (!dynamic_cast<const mu::engraving::ChordRest *>(element))
    return false;
  const auto chord = dynamic_cast<const mu::engraving::Chord *>(element);
  Finally finally{[&] { prevWasRest = chord == nullptr; }};
  if (!chord)
    return !prevWasRest;
  return HasUntiedNotes(*chord);
}

bool IsVisible(const mu::engraving::Staff &staff) {
  return staff.visible() &&
         !staff.staffType(mu::engraving::Fraction(0, 1))->isMuted();
}

// At the moment we are not flexible at all: we look for the first part that has
// two staves and assume this is what we want to play.
std::optional<size_t>
GetRightHandStaff(const std::vector<mu::engraving::RepeatSegment *> &repeats,
                  size_t nScoreTracks) {
  for (const auto &repeat : repeats)
    for (const auto &measure : repeat->measureList())
      for (const auto &segment : measure->segments())
        for (auto track = 0u; track < nScoreTracks; ++track)
          if (const auto chord = dynamic_cast<const mu::engraving::Chord *>(
                  segment.element(track))) {
            const auto staves = chord->part()->staves();
            if (staves.size() == 2 && IsVisible(*chord->staff()))
              return chord->staff()->idx();
          }
  return std::nullopt;
}

auto GetChordSequence(mu::engraving::Score &score,
                      mu::notation::INotationInteraction &interaction,
                      size_t staffIdx, int voice) {
  std::vector<ChordPtr> sequence;
  auto prevWasRest = false;
  const auto &repeats = score.repeatList();
  std::for_each(
      repeats.begin(), repeats.end(),
      [&](const mu::engraving::RepeatSegment *repeatSegment) {
        const auto &museMeasures = repeatSegment->measureList();
        std::for_each(
            museMeasures.begin(), museMeasures.end(),
            [&](const mu::engraving::Measure *measure) {
              const auto &museSegments = measure->segments();
              for (const auto &museSegment : museSegments)
                if (TakeIt(museSegment, staffIdx, voice, prevWasRest))
                  sequence.push_back(std::make_shared<MuseChord>(
                      score, interaction, museSegment, staffIdx, voice));
            });
      });
  return sequence;
}

} // namespace

std::unique_ptr<OrchestrionSequencer>
OrchestrionSequencerFactory::CreateSequencer(
    mu::engraving::Score &score,
    mu::notation::INotationInteraction &interaction) {
  const auto rightHandStaff =
      GetRightHandStaff(score.repeatList(), score.ntracks());
  if (!rightHandStaff.has_value())
    return nullptr;
  Staff rightHand;
  Staff leftHand;
  for (auto v = 0; v < numVoices; ++v) {
    if (auto sequence =
            GetChordSequence(score, interaction, *rightHandStaff, v);
        !sequence.empty())
      rightHand.emplace(v, std::move(sequence));
    if (auto sequence =
            GetChordSequence(score, interaction, *rightHandStaff + 1, v);
        !sequence.empty())
      leftHand.emplace(v, std::move(sequence));
  }
  return std::make_unique<OrchestrionSequencer>(std::move(rightHand),
                                                std::move(leftHand));
}

NoteEvent ToDgkNoteEvent(const muse::midi::Event &museEvent) {
  const auto type = museEvent.opcode() == muse::midi::Event::Opcode::NoteOn
                        ? NoteEvent::Type::noteOn
                        : NoteEvent::Type::noteOff;
  const int channel = museEvent.channel();
  const int pitch = museEvent.note();
  const float velocity = museEvent.velocity() / 128.0f;
  return NoteEvent{type, channel, pitch, velocity};
}

muse::midi::Event ToMuseMidiEvent(const NoteEvent &dgkEvent) {
  muse::midi::Event museEvent(dgkEvent.type == dgk::NoteEvent::Type::noteOn
                                  ? muse::midi::Event::Opcode::NoteOn
                                  : muse::midi::Event::Opcode::NoteOff,
                              muse::midi::Event::MessageType::ChannelVoice10);
  museEvent.setChannel(dgkEvent.channel);
  museEvent.setNote(dgkEvent.pitch);
  museEvent.setVelocity(dgkEvent.velocity * 128);
  return museEvent;
}
} // namespace dgk