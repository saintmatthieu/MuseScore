#pragma once

#include "IChord.h"

namespace dgk {
class VoiceBlank : public IChord {
public:
  VoiceBlank(Tick tick);

private:
  bool IsChord() const override;
  std::vector<int> GetPitches() const override;
  Tick GetTick() const override;
  void SetHighlight(bool value) override;
  void ScrollToYou() const override;

  const Tick m_tick;
};
} // namespace dgk