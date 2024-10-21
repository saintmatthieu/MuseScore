#pragma once

#include <optional>
#include <vector>

namespace mu {

namespace engraving {
class Chord;
class Measure;
class Note;
class Score;
class Segment;
class RepeatSegment;
} // namespace engraving

class ChordRestIterator {
public:
  static std::vector<engraving::Chord *>
  getChords(const engraving::Segment &segment,
            const mu::engraving::Score &score,
            const std::optional<bool> &rightHand);

  static std::vector<engraving::Note *>
  getUntiedNotes(const engraving::Chord &chord);

  using RepeatSegmentVector = std::vector<engraving::RepeatSegment *>;

  ChordRestIterator(const engraving::Score &score,
                    const RepeatSegmentVector &repeatList,
                    std::optional<bool> leftHand);

  engraving::Segment *next(uint8_t midiPitch);
  //! If there are repeats, goes to the first iteration of that repeat.
  void goTo(engraving::Segment *segment);
  const std::optional<bool> &isRightHand() const { return m_rightHand; }

private:
  static bool skipSegment(const engraving::Segment &segment,
                          const engraving::Score &score,
                          const std::optional<bool> &leftHand);

  void goToStart();
  engraving::Segment *nextRepeatSegment();
  engraving::Segment *nextMeasureSegment();

  const engraving::Score &m_score;
  const RepeatSegmentVector &m_repeatList;
  const std::optional<bool> m_rightHand;
  RepeatSegmentVector::const_iterator m_repeatSegmentIt;
  std::vector<const engraving::Measure *>::const_iterator m_measureIt;
  engraving::Segment *m_pSegment;
};
} // namespace mu
