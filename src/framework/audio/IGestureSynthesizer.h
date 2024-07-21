#pragma once

#include <vector>

namespace dgk {
struct NoteEvent;

class IGestureSynthesizer {
public:
  virtual ~IGestureSynthesizer() = default;
  virtual void processNoteEvents(const std::vector<NoteEvent> &) {}
};
} // namespace dgk
