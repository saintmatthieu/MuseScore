#pragma once

#include "audio/GestureSynthesizer.h"
#include "internal/apitypes.h"

namespace muse::musesampler {
struct MuseSamplerLibHandler;
}

namespace dgk {
class MuseGestureSynthesizer : public dgk::GestureSynthesizer {
public:
  MuseGestureSynthesizer(muse::audio::synth::ISynthesizerPtr synth,
                         muse::musesampler::MuseSamplerLibHandler &samplerLib,
                         int instrumentId);
  ~MuseGestureSynthesizer() override;
  void processEventVariant(const dgk::EventVariant &noteEvents) override;
  void doSetup(const mpe::PlaybackData &playbackData) override;
  void doProcess(float *buffer,
                 muse::audio::samples_t samplesPerChannel) override;
  void doSetSampleRate(unsigned int sampleRate) override;
  void doFlushSound() override;
  void doSetIsActive(bool arg) override;

private:
  void extractOutputSamples(muse::audio::samples_t samples, float *output);

  muse::musesampler::MuseSamplerLibHandler &m_samplerLib;
  const int m_instrumentId;
  ms_MuseSampler m_sampler = nullptr;
  ms_Track m_track = nullptr;

  std::vector<float> m_leftChannel;
  std::vector<float> m_rightChannel;
  std::array<float *, 2> m_internalBuffer;
  ms_OutputBuffer m_bus;
};
} // namespace dgk