#pragma once

#include "Orchestrion/OrchestrionSequencer.h"
#include "Orchestrion/OrchestrionTypes.h"
#include "midi/midievent.h"
#include <memory>
#include <vector>

namespace mu::engraving {
class Score;
}

namespace mu::notation {
class INotationInteraction;
}

namespace dgk {
class OrchestrionSequencerFactory {
public:
  static std::unique_ptr<OrchestrionSequencer>
  CreateSequencer(mu::engraving::Score &score,
                  mu::notation::INotationInteraction &interaction,
                  OrchestrionSequencer::MidiOutCb cb);
};

NoteEvent ToDgkNoteEvent(const muse::midi::Event &museEvent);
muse::midi::Event ToMuseMidiEvent(const NoteEvent &noteEvent);
} // namespace dgk