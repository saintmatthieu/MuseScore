#pragma once

#include "OrchestrionTypes.h"
#include <async/channel.h>
#include <modularity/imoduleinterface.h>
#include <optional>

namespace dgk
{
class IOrchestrionSequencer : MODULE_EXPORT_INTERFACE
{
  INTERFACE_ID(IOrchestrionSequencer);

public:
  virtual ~IOrchestrionSequencer() = default;

  virtual void OnInputEvent(const NoteEvent &inputEvent) = 0;
  virtual void GoToTick(int tick) = 0;
  virtual int GetTrack() const = 0;
  virtual muse::async::Channel<int /* track */, ChordActivationChange>
  ChordActivationChanged() const = 0;
};
} // namespace dgk