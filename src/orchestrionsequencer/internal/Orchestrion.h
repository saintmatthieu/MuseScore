#pragma once

#include "IOrchestrion.h"
#include "OrchestrionSequencer.h"
#include "playback/iplaybackcontroller.h"
#include <async/asyncable.h>
#include <audio/isynthresolver.h>
#include <context/iglobalcontext.h>
#include <midi/imidioutport.h>

namespace dgk
{
class Orchestrion : public IOrchestrion,
                    public muse::async::Asyncable,
                    public muse::Injectable
{
  muse::Inject<mu::playback::IPlaybackController> playbackController = {this};
  muse::Inject<mu::context::IGlobalContext> globalContext = {this};
  muse::Inject<muse::midi::IMidiOutPort> midiOutPort = {this};
  muse::Inject<muse::audio::synth::ISynthResolver> synthResolver = {this};

public:
  void init();

private:
  IOrchestrionSequencer *sequencer() override;

private:
  std::unique_ptr<IOrchestrionSequencer> m_sequencer;
};
} // namespace dgk