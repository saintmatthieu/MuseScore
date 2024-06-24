#pragma once

#include <vector>

namespace dgk {
class IChord {
public:
  virtual ~IChord() = default;
  virtual bool IsChord() const = 0;
  virtual std::vector<int> GetPitches() const = 0;
  virtual int GetTick() const = 0; // Doesn't account for repeats, just the
                                   // visual position in the score.
  virtual void SetHighlight(bool value) = 0;
  virtual void ScrollToYou() const = 0;
};
} // namespace dgk