#pragma once

#include "IChord.h"

namespace dgk {
class VoiceBlank : public IChord {
public:
  VoiceBlank(int tick);

private:
  std::vector<int> GetPitches() const override;
  int GetTick() const override;
  void SetHighlight(bool value) override;
  void ScrollToYou() const override;

  const int m_tick;
};
} // namespace dgk