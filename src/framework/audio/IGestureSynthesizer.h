#pragma once

#include "OrchestrionEventTypes.h"

namespace dgk
{
class IGestureSynthesizer
{
public:
  virtual ~IGestureSynthesizer() = default;
  virtual void processEventVariant(const dgk::EventVariant &) {}
};
} // namespace dgk
