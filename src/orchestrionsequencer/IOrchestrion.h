#pragma once

#include <modularity/imoduleinterface.h>

namespace dgk
{
class IOrchestrionSequencer;

class IOrchestrion : MODULE_EXPORT_INTERFACE
{
  INTERFACE_ID(IOrchestrion);

public:
  virtual ~IOrchestrion() = default;

  virtual IOrchestrionSequencer* sequencer() = 0;
};
} // namespace dgk