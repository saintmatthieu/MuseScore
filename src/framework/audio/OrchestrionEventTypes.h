#pragma once

#include <variant>
#include <vector>

namespace dgk
{
struct NoteEvent
{
  enum class Type
  {
    noteOn,
    noteOff
  };

  Type type = Type::noteOn;
  int channel = 0;
  int pitch = 0;
  float velocity = 0.f;
};

using NoteEvents = std::vector<NoteEvent>;

struct PedalEvent
{
  int channel = 0;
  bool on = false;
};

using EventVariant = std::variant<NoteEvents, PedalEvent>;

} // namespace dgk