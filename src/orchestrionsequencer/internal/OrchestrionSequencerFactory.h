#pragma once

#include "OrchestrionSequencer.h"
#include "OrchestrionTypes.h"
#include "midi/midievent.h"
#include "playback/iplaybackcontroller.h"
#include <memory>
#include <vector>

namespace mu::engraving
{
class Score;
}

namespace mu::notation
{
class IMasterNotation;
}

namespace dgk
{
class OrchestrionSequencerFactory
{
public:
  static std::unique_ptr<IOrchestrionSequencer> CreateSequencer(
      mu::notation::IMasterNotation &masterNotation,
      const mu::playback::IPlaybackController::InstrumentTrackIdMap &,
      IOrchestrionSequencer::MidiOutCb cb);
};

muse::midi::Event ToMuseMidiEvent(const NoteEvent &noteEvent);
muse::midi::Event ToMuseMidiEvent(const PedalEvent &pedalEvent);
} // namespace dgk