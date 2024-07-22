#pragma once

#include <fluidsynth/types.h>

#include "audio/GestureSynthesizer.h"
#include "soundmapping.h"

namespace dgk {
class FluidGestureSynthesizer : public dgk::GestureSynthesizer {
public:
  FluidGestureSynthesizer(muse::audio::synth::ISynthesizerPtr synth,
                          std::vector<mu::io::path_t> soundFonts,
                          std::optional<midi::Program> preset);
  ~FluidGestureSynthesizer() override;
  void
  processNoteEvents(const std::vector<dgk::NoteEvent> &noteEvents) override;
  void doSetup(const mpe::PlaybackSetupData &) override;
  void doProcess(float *buffer,
                 muse::audio::samples_t samplesPerChannel) override;
  void doSetSampleRate(unsigned int sampleRate) override;
  void doFlushSound() override;
  void doSetIsActive(bool arg) override;

private:
  void destroySynth();
  void addSoundFonts();

  const std::vector<mu::io::path_t> m_soundFonts;
  const std::optional<midi::Program> m_preset;
  mpe::PlaybackSetupData m_setupData;
  muse::audio::ChannelMap m_channels;
  fluid_synth_t *m_fluidSynth = nullptr;
  fluid_settings_t *m_fluidSettings = nullptr;
  uint64_t m_sampleRate = 0;
};
} // namespace dgk