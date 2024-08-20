#pragma once

#include "orchestrion/OrchestrionTypes.h"
#include <vector>

namespace dgk {
struct NoteEvent;

class IGestureSynthesizer {
public:
  virtual ~IGestureSynthesizer() = default;
  virtual void processEventVariant(const dgk::EventVariant&) {}
};
} // namespace dgk
