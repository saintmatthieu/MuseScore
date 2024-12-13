#pragma once

#include "OrchestrionTypes.h"
#include <modularity/imoduleinterface.h>

namespace dgk
{
class IComputerKeyboardMidiController : MODULE_EXPORT_INTERFACE
{
  INTERFACE_ID(IComputerKeyboardMidiController);

public:
  virtual ~IComputerKeyboardMidiController() = default;

  virtual void onAltPlusLetter(char) = 0;
  virtual void onReleasedLetter(char) = 0;
};
} // namespace dgk