#pragma once

#include "OrchestrionTypes.h"
#include <modularity/imoduleinterface.h>
#include <optional>

namespace dgk
{
class IOrchestrionSequencer : MODULE_EXPORT_INTERFACE
{
  INTERFACE_ID(IOrchestrionSequencer);

public:
  using MidiOutCb =
      std::function<void(const std::variant<NoteEvents, PedalEvent> &)>;

  virtual ~IOrchestrionSequencer() = default;

  virtual void OnInputEvent(const NoteEvent &inputEvent) = 0;
  virtual void GoToTick(int tick) = 0;
  virtual int GetTrack() const = 0;
};
} // namespace dgk