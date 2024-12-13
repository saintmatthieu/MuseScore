#include "Orchestrion.h"
#include "OrchestrionSequencerFactory.h"
#include <engraving/dom/masterscore.h>

namespace dgk
{
void Orchestrion::init()
{
  playbackController()->isPlayAllowedChanged().onNotify(
      this,
      [&]()
      {
        const auto masterNotation = globalContext()->currentMasterNotation();
        if (!masterNotation)
        {
          m_sequencer.reset();
          return;
        }
        const auto &map = playbackController()->instrumentTrackIdMap();
        if (map.empty())
        {
          m_sequencer.reset();
          return;
        }
        m_sequencer = OrchestrionSequencerFactory::CreateSequencer(
            *masterNotation, map,
            [this](const std::variant<NoteEvents, PedalEvent> &event)
            {
              if (std::holds_alternative<NoteEvents>(event))
              {
                const auto &events = std::get<NoteEvents>(event);
                std::for_each(
                    events.begin(), events.end(), [&](const NoteEvent &event)
                    { midiOutPort()->sendEvent(ToMuseMidiEvent(event)); });
              }
              else if (std::holds_alternative<PedalEvent>(event))
              {
                const auto &pedalEvent = std::get<PedalEvent>(event);
                midiOutPort()->sendEvent(ToMuseMidiEvent(pedalEvent));
              }
              synthResolver()->postEventVariant(m_sequencer->GetTrack(), event);
            });
      });
}

IOrchestrionSequencer *Orchestrion::sequencer() { return m_sequencer.get(); }
} // namespace dgk