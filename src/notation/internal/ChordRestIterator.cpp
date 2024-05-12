#include "ChordRestIterator.h"
#include "engraving/dom/note.h"
#include "engraving/dom/rest.h"
#include "engraving/dom/score.h"
#include "engraving/dom/segment.h"

namespace mu {

std::vector<Chord *>
ChordRestIterator::getChords(const Segment &segment,
                             const mu::engraving::Score &score,
                             const std::optional<bool> &rightHand) {
  std::vector<Chord *> chords;
  auto trackId = 0;
  while (trackId < score.ntracks()) {
    if (const auto chord = dynamic_cast<Chord *>(segment.element(trackId))) {
      const auto part = chord->part();
      const auto staves = part->staves();
      const auto staff = chord->staff();
      const auto isCorrectHand =
          !rightHand.has_value() || *rightHand == isRightHandChord(*chord);
      if (staff->visible() &&
          !staff->staffType(mu::engraving::Fraction(0, 1))->isMuted() &&
          isCorrectHand)
        chords.push_back(chord);
    }
    ++trackId;
  }
  return chords;
}

std::vector<Note *> ChordRestIterator::getUntiedNotes(const Chord &chord) {
  std::vector<Note *> notes;
  for (Note *note : chord.notes()) {
    if (!note->tieBack()) {
      notes.push_back(note);
    }
  }
  return notes;
}

ChordRestIterator::ChordRestIterator(const engraving::Score &score,
                                     const RepeatSegmentVector &repeatList,
                                     std::optional<bool> rightHand)
    : m_score{score}, m_repeatList{repeatList}, m_rightHand{
                                                    std::move(rightHand)} {
  goToStart();
}

engraving::Segment *ChordRestIterator::next(uint8_t midiPitch) {
  // If left-hand isn't set, accept any note. Else, if left-hand, accept notes
  // below C4, and if right-hand, accept notes above C4.
  if (m_rightHand.has_value() && (*m_rightHand != (midiPitch >= 60))) {
    return nullptr;
  }

  while (m_repeatSegmentIt != m_repeatList.end()) {
    if (auto segment = nextRepeatSegment())
      return segment;
    m_repeatSegmentIt++;
    if (m_repeatSegmentIt == m_repeatList.end()) {
      // Finished !
      goToStart();
      return nullptr;
    }
    m_measureIt = (*m_repeatSegmentIt)->measureList().begin();
    m_pSegment = (*m_measureIt)->first();
  }
  return nullptr;
}

void ChordRestIterator::goTo(engraving::Segment *segment) {
  if (m_repeatSegmentIt == m_repeatList.end()) {
    m_repeatSegmentIt = m_repeatList.begin();
  }
  // Go to beginning of repeat and start search from there.
  m_measureIt = (*m_repeatSegmentIt)->measureList().begin();
  while (m_repeatSegmentIt != m_repeatList.end()) {
    m_measureIt = (*m_repeatSegmentIt)->measureList().begin();
    while (m_measureIt != (*m_repeatSegmentIt)->measureList().end()) {
      const auto &segments = (*m_measureIt)->segments();
      if (std::find_if(segments.begin(), segments.end(),
                       [segment](const engraving::Segment &candidate) {
                         return segment == &candidate;
                       }) != segments.end()) {
        m_pSegment = segment;
        return;
      }
      ++m_measureIt;
    }
    ++m_repeatSegmentIt;
  }
}

bool ChordRestIterator::skipSegment(const engraving::Segment &segment,
                                    const engraving::Score &score,
                                    const std::optional<bool> &rightHand) {
  if (segment.segmentType() != mu::engraving::SegmentType::ChordRest)
    return true;
  const auto chords = getChords(segment, score, rightHand);
  if (chords.empty())
    return true;
  return std::all_of(chords.begin(), chords.end(), [](const Chord *chord) {
    return getUntiedNotes(*chord).empty();
  });
}

void ChordRestIterator::goToStart() {
  m_repeatSegmentIt = m_repeatList.begin();
  m_measureIt = (*m_repeatSegmentIt)->measureList().begin();
  m_pSegment = (*m_measureIt)->first();
}

engraving::Segment *ChordRestIterator::nextRepeatSegment() {
  assert(m_repeatSegmentIt != m_repeatList.end());
  while (m_measureIt != (*m_repeatSegmentIt)->measureList().end()) {
    if (auto segment = nextMeasureSegment())
      return segment;
    m_measureIt++;
    if (m_measureIt != (*m_repeatSegmentIt)->measureList().end())
      m_pSegment = (*m_measureIt)->first();
  }
  return nullptr;
}

engraving::Segment *ChordRestIterator::nextMeasureSegment() {
  assert(m_pSegment);
  while (m_pSegment &&
         ChordRestIterator::skipSegment(*m_pSegment, m_score, m_rightHand))
    m_pSegment = m_pSegment->next();
  auto segment = m_pSegment;
  if (m_pSegment)
    m_pSegment = m_pSegment->next();
  return segment;
}
} // namespace mu
