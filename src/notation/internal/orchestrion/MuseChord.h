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
            const mu::engraving::Segment &segment, size_t staffIdx, int voice,
            int measurePlaybackTick);

  bool IsChord() const override;
  Tick GetBeginTick() const override;
  Tick GetEndTick() const override;

private:
  std::vector<int> GetPitches() const override;
  void SetHighlight(bool value) override;
  void ScrollToYou() const override;
  Tick GetChordEndTick() const;
  Tick GetRestEndTick() const;

  std::vector<mu::engraving::Note *> GetNotes() const;

  const Tick m_tick;
  const int m_track;
  const bool m_isChord;
  const size_t m_staffIdx;
  const int m_voice;

  mu::engraving::Score &m_score;
  mu::notation::INotationInteraction &m_notationInteraction;
  const mu::engraving::Segment &m_segment;
};
} // namespace dgk