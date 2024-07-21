#include "MuseGestureSynthesizer.h"
#include "internal/libhandler.h"
#include "orchestrion/OrchestrionTypes.h"

MuseGestureSynthesizer::MuseGestureSynthesizer(
    muse::audio::synth::ISynthesizerPtr synth,
    muse::musesampler::MuseSamplerLibHandler &samplerLib, int instrumentId)
    : GestureSynthesizer{std::move(synth)}, m_samplerLib(samplerLib),
      m_instrumentId(instrumentId) {}

MuseGestureSynthesizer::~MuseGestureSynthesizer() {
  if (m_sampler) {
    m_samplerLib.destroy(m_sampler);
  }
}

void MuseGestureSynthesizer::processNoteEvents(
    const std::vector<dgk::NoteEvent> &noteEvents) {
  // Actually there should be one unique velocity for all events. Review this.
  const auto velocity = noteEvents.empty() ? 0 : noteEvents.front().velocity;
  constexpr auto timestamp = 0LL;
  m_samplerLib.addDynamicsEvent(m_sampler, m_track, timestamp, velocity);
  for (const auto &noteEvent : noteEvents)
    if (noteEvent.type == dgk::NoteEvent::Type::noteOn) {
      muse::musesampler::AuditionStartNoteEvent noteOn;
      constexpr auto centsOffset = 0;
      constexpr auto presets_cstr = "";
      constexpr auto textArticulation_cstr = "";
      noteOn.msEvent = {noteEvent.pitch,          centsOffset,
                        ms_NoteArticulation_None, ms_NoteHead_Normal,
                        noteEvent.velocity,       presets_cstr,
                        textArticulation_cstr};
      noteOn.msTrack = m_track;
      m_samplerLib.startAuditionNote(m_sampler, noteOn.msTrack, noteOn.msEvent);
    } else
      m_samplerLib.stopAuditionNote(m_sampler, m_track, {noteEvent.pitch});
}

void MuseGestureSynthesizer::doSetup() {
  m_track = m_samplerLib.addTrack(m_sampler, m_instrumentId);
}

void MuseGestureSynthesizer::doFlushSound() {
  m_samplerLib.allNotesOff(m_sampler);
}

void MuseGestureSynthesizer::doSetIsActive(bool arg) {
  m_samplerLib.setPlaying(m_sampler, arg);
}

void MuseGestureSynthesizer::doSetSampleRate(unsigned int sampleRate) {
  if (m_sampler)
    return;

  m_sampler = m_samplerLib.create();
  constexpr auto renderStep = 512;
  m_samplerLib.initSampler2(m_sampler, sampleRate, renderStep,
                            AUDIO_CHANNELS_COUNT);

  m_leftChannel.resize(renderStep);
  m_rightChannel.resize(renderStep);

  m_bus._num_channels = AUDIO_CHANNELS_COUNT;
  m_bus._num_data_pts = renderStep;

  m_internalBuffer[0] = m_leftChannel.data();
  m_internalBuffer[1] = m_rightChannel.data();
  m_bus._channels = m_internalBuffer.data();
}

void MuseGestureSynthesizer::doProcess(
    float *buffer, muse::audio::samples_t samplesPerChannel) {
  constexpr auto currentPosition = 0;
  m_samplerLib.process(m_sampler, m_bus, currentPosition);
  extractOutputSamples(samplesPerChannel, buffer);
}

void MuseGestureSynthesizer::extractOutputSamples(
    muse::audio::samples_t samples, float *output) {
  for (muse::audio::samples_t sampleIndex = 0; sampleIndex < samples;
       ++sampleIndex) {
    size_t offset = sampleIndex * m_bus._num_channels;

    for (muse::audio::audioch_t audioChannelIndex = 0;
         audioChannelIndex < m_bus._num_channels; ++audioChannelIndex) {
      float sample = m_bus._channels[audioChannelIndex][sampleIndex];
      output[offset + audioChannelIndex] += sample;
    }
  }
}
