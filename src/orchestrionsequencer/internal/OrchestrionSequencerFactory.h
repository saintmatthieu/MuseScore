#pragma once

#include "OrchestrionTypes.h"
#include "ScoreAnimation/IChordRegistry.h"
#include "playback/iplaybackcontroller.h"
#include <memory>
#include <modularity/ioc.h>
#include <vector>

namespace mu::notation
{
class IMasterNotation;
}

namespace dgk
{
class IOrchestrionSequencer;

class OrchestrionSequencerFactory : public muse::Injectable
{
  muse::Inject<orchestrion::IChordRegistry> chordRegistry;

public:
  std::unique_ptr<IOrchestrionSequencer> CreateSequencer(
      mu::notation::IMasterNotation &masterNotation,
      const mu::playback::IPlaybackController::InstrumentTrackIdMap &,
      MidiOutCb cb);
};

muse::midi::Event ToMuseMidiEvent(const NoteEvent &noteEvent);
muse::midi::Event ToMuseMidiEvent(const PedalEvent &pedalEvent);
} // namespace dgk