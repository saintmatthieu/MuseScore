#pragma once

#include "Orchestrion/IChord.h"

namespace mu::engraving {
class Note;
class Score;
class Segment;
class Rest;
} // namespace mu::engraving

namespace mu::notation {
class INotationInteraction;
} // namespace mu::notation

namespace dgk {
class MuseChord : public IChord {
public:
  MuseChord(mu::engraving::Score &score,
            mu::notation::INotationInteraction &interaction,
            const mu::engraving::Segment &segment, size_t staffIdx, int voice);

  bool IsChord() const override;
  int GetTick() const override;
  int GetEndTick() const;

private:
  std::vector<int> GetPitches() const override;
  void SetHighlight(bool value) override;
  void ScrollToYou() const override;
  int GetChordEndTick() const;
  int GetRestEndTick() const;

  std::vector<mu::engraving::Note *> GetNotes() const;

  mu::engraving::Score &m_score;
  mu::notation::INotationInteraction &m_notationInteraction;
  const mu::engraving::Segment &m_segment;
  const size_t m_staffIdx;
  const int m_voice;
  const int m_track;
  const bool m_isChord;
};
} // namespace dgk