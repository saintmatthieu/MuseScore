#pragma once

#include "audio/ISynthesizer.h"

namespace dgk {
using namespace muse;
using namespace mu;

class GestureSynthesizer : public muse::audio::synth::ISynthesizer {
public:
  virtual ~GestureSynthesizer() = default;

  GestureSynthesizer(muse::audio::synth::ISynthesizerPtr synth);

  virtual void doSetup(const muse::audio::PlaybackSetupData&) = 0;
  virtual void doProcess(float *buffer, muse::audio::samples_t samplesPerChannel) = 0;
  virtual void doSetSampleRate(unsigned int sampleRate) = 0;
  virtual void doFlushSound() = 0;
  virtual void doSetIsActive(bool arg) = 0;

  // ISynthesizer
  std::string name() const override;
  muse::audio::AudioSourceType type() const override;
  bool isValid() const override;
  void setup(const mpe::PlaybackData &playbackData) override;
  const audio::AudioInputParams &params() const override;
  muse::async::Channel<audio::AudioInputParams> paramsChanged() const override;
  muse::audio::msecs_t playbackPosition() const override;
  void setPlaybackPosition(const muse::audio::msecs_t newPosition) override;
  void revokePlayingNotes() override;
  void flushSound() override;

  // IAudioSource
  bool isActive() const override;
  void setIsActive(bool arg) override;
  void setSampleRate(unsigned int sampleRate) override;
  unsigned int audioChannelsCount() const override;
  muse::async::Channel<unsigned int> audioChannelsCountChanged() const override;
  muse::audio::samples_t process(float *buffer, muse::audio::samples_t samplesPerChannel) override;

private:
  const muse::audio::synth::ISynthesizerPtr m_synth;
};
} // namespace dgk