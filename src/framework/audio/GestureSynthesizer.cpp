#include "GestureSynthesizer.h"
#include <algorithm>

namespace dgk {
GestureSynthesizer::GestureSynthesizer(
    muse::audio::synth::ISynthesizerPtr synth)
    : m_synth{std::move(synth)} {}

std::string GestureSynthesizer::name() const { return m_synth->name(); }

muse::audio::AudioSourceType GestureSynthesizer::type() const {
  return m_synth->type();
}

bool GestureSynthesizer::isValid() const { return m_synth->isValid(); }

void GestureSynthesizer::setup(const mpe::PlaybackData &playbackData) {
  m_synth->setup(playbackData);
  doSetup(playbackData);
}

const audio::AudioInputParams &GestureSynthesizer::params() const {
  return m_synth->params();
}

muse::async::Channel<audio::AudioInputParams>
GestureSynthesizer::paramsChanged() const {
  return m_synth->paramsChanged();
}

muse::audio::msecs_t GestureSynthesizer::playbackPosition() const {
  return m_synth->playbackPosition();
}

void GestureSynthesizer::setPlaybackPosition(
    const muse::audio::msecs_t newPosition) {
  m_synth->setPlaybackPosition(newPosition);
}

void GestureSynthesizer::revokePlayingNotes() { m_synth->revokePlayingNotes(); }

void GestureSynthesizer::flushSound() {
  m_synth->flushSound();
  doFlushSound();
}

bool GestureSynthesizer::isActive() const { return m_synth->isActive(); }

void GestureSynthesizer::setIsActive(bool arg) { m_synth->setIsActive(arg); }

void GestureSynthesizer::setSampleRate(unsigned int sampleRate) {
  m_buffer.resize(
      sampleRate *
      audioChannelsCount()); // One second - should be more that enough.
  doSetSampleRate(sampleRate);
  m_synth->setSampleRate(sampleRate);
}

unsigned int GestureSynthesizer::audioChannelsCount() const {
  return m_synth->audioChannelsCount();
}

muse::async::Channel<unsigned int>
GestureSynthesizer::audioChannelsCountChanged() const {
  return m_synth->audioChannelsCountChanged();
}

muse::audio::samples_t
GestureSynthesizer::process(float *buffer,
                            muse::audio::samples_t samplesPerChannel) {
  const auto processed = m_synth->process(buffer, samplesPerChannel);
  const auto numChannels = audioChannelsCount();
  m_buffer.resize(processed * numChannels);
  doProcess(m_buffer.data(), processed);
  for (auto i = 0; i < processed * numChannels; ++i)
    buffer[i] += std::clamp(m_buffer[i], -1.f, 1.f);
  return processed;
}
} // namespace dgk
