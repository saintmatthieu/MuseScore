#include <fluidsynth.h>

#include "FluidGestureSynthesizer.h"
#include "orchestrionsequencer/OrchestrionTypes.h"
#include "sfcachedloader.h"
#include <cassert>

namespace dgk {
namespace {
constexpr double FLUID_GLOBAL_VOLUME_GAIN = 4.8;
constexpr int DEFAULT_MIDI_VOLUME = 100;
constexpr muse::audio::msecs_t MIN_NOTE_LENGTH = 10;
constexpr unsigned int FLUID_AUDIO_CHANNELS_PAIR = 1;
constexpr unsigned int FLUID_AUDIO_CHANNELS_COUNT =
    FLUID_AUDIO_CHANNELS_PAIR * 2;
} // namespace
FluidGestureSynthesizer::FluidGestureSynthesizer(
    muse::audio::synth::ISynthesizerPtr synth,
    std::vector<mu::io::path_t> soundFonts, std::optional<midi::Program> preset)
    : GestureSynthesizer{std::move(synth)},
      m_soundFonts{std::move(soundFonts)}, m_preset{std::move(preset)} {}

FluidGestureSynthesizer::~FluidGestureSynthesizer() { destroySynth(); }

void FluidGestureSynthesizer::destroySynth() {
  if (m_fluidSynth) {
    delete_fluid_synth(m_fluidSynth);
    m_fluidSynth = nullptr;
  }
  if (m_fluidSettings) {
    delete_fluid_settings(m_fluidSettings);
    m_fluidSettings = nullptr;
  }
}

void FluidGestureSynthesizer::processEventVariant(
    const dgk::EventVariant &event) {
  if (std::holds_alternative<dgk::NoteEvents>(event))
    for (const auto &evt : std::get<dgk::NoteEvents>(event)) {
      switch (evt.type) {
      case NoteEvent::Type::noteOn:
        fluid_synth_noteon(m_fluidSynth, evt.channel, evt.pitch,
                           evt.velocity * 127 + .5f);
        break;
      case NoteEvent::Type::noteOff:
        fluid_synth_noteoff(m_fluidSynth, evt.channel, evt.pitch);
        break;
      default:
        assert(false);
      }
    }
  else if (std::holds_alternative<dgk::PedalEvent>(event))
  {
    const auto& pedalEvent = std::get<dgk::PedalEvent>(event);
    fluid_synth_cc(m_fluidSynth, pedalEvent.channel, 0x40,
                   pedalEvent.on ? 127 : 0);
  }
}

void FluidGestureSynthesizer::doSetup(const mpe::PlaybackData &playbackData) {

  assert(m_sampleRate != 0);
  if (m_sampleRate == 0)
    return;

  m_channels.init(playbackData.setupData, m_preset);
  fluid_synth_activate_key_tuning(m_fluidSynth, 0, 0, "standard", NULL, true);

  const auto setupChannel = [this](const midi::channel_t channelIdx,
                                   const midi::Program &program) {
    fluid_synth_set_interp_method(m_fluidSynth, channelIdx,
                                  FLUID_INTERP_DEFAULT);
    fluid_synth_pitch_wheel_sens(m_fluidSynth, channelIdx, 24);
    fluid_synth_bank_select(m_fluidSynth, channelIdx, program.bank);
    fluid_synth_program_change(m_fluidSynth, channelIdx, program.program);
    fluid_synth_cc(m_fluidSynth, channelIdx, 7, DEFAULT_MIDI_VOLUME);
    fluid_synth_cc(m_fluidSynth, channelIdx, 74, 0);
    fluid_synth_set_portamento_mode(m_fluidSynth, channelIdx,
                                    FLUID_CHANNEL_PORTAMENTO_MODE_EACH_NOTE);
    fluid_synth_set_legato_mode(m_fluidSynth, channelIdx,
                                FLUID_CHANNEL_LEGATO_MODE_RETRIGGER);
    fluid_synth_activate_tuning(m_fluidSynth, channelIdx, 0, 0, 0);
  };

  for (const auto &voice : m_channels.data())
    for (const auto &pair : voice.second) {
      const auto &[ch, program] = pair.second;
      setupChannel(ch, program);
    }

  m_channels.channelAdded.onReceive(this, setupChannel);
  for (const auto &[_, eventList] : playbackData.originEvents)
    for (const auto &event : eventList)
      if (std::holds_alternative<muse::mpe::NoteEvent>(event))
        // Will call `setupChannel` for each new channel.
        m_channels.resolveChannelForEvent(
            std::get<muse::mpe::NoteEvent>(event));
}

void FluidGestureSynthesizer::doFlushSound() {}

void FluidGestureSynthesizer::doSetIsActive(bool) {}

void FluidGestureSynthesizer::doSetSampleRate(unsigned int sampleRate) {

  if (m_sampleRate == sampleRate || sampleRate == 0)
    return;

  destroySynth();

  m_sampleRate = sampleRate;

  m_fluidSettings = new_fluid_settings();
  fluid_settings_setnum(m_fluidSettings, "synth.gain",
                        FLUID_GLOBAL_VOLUME_GAIN);
  fluid_settings_setint(m_fluidSettings, "synth.audio-channels",
                        FLUID_AUDIO_CHANNELS_PAIR); // 1 pair of audio channels
  fluid_settings_setint(m_fluidSettings, "synth.lock-memory", 0);
  fluid_settings_setint(m_fluidSettings, "synth.threadsafe-api", 0);
  fluid_settings_setint(m_fluidSettings, "synth.midi-channels", 16);
  fluid_settings_setint(m_fluidSettings, "synth.dynamic-sample-loading", 1);
  fluid_settings_setint(m_fluidSettings, "synth.polyphony", 512);
  fluid_settings_setint(m_fluidSettings, "synth.min-note-length",
                        MIN_NOTE_LENGTH);
  fluid_settings_setint(m_fluidSettings, "synth.chorus.active", 0);
  fluid_settings_setint(m_fluidSettings, "synth.reverb.active", 0);
  fluid_settings_setstr(m_fluidSettings, "audio.sample-format", "float");
  fluid_settings_setnum(m_fluidSettings, "synth.sample-rate",
                        static_cast<double>(m_sampleRate));

  m_fluidSynth = new_fluid_synth(m_fluidSettings);
  auto sfloader = new_fluid_sfloader(muse::audio::synth::loadSoundFont,
                                     delete_fluid_sfloader);
  fluid_sfloader_set_data(sfloader, m_fluidSettings);
  fluid_synth_add_sfloader(m_fluidSynth, sfloader);

  std::for_each(m_soundFonts.begin(), m_soundFonts.end(),
                [this](const auto &sfont) {
                  fluid_synth_sfload(m_fluidSynth, sfont.c_str(), 0);
                });
}

void FluidGestureSynthesizer::doProcess(
    float *buffer, muse::audio::samples_t samplesPerChannel) {
  fluid_synth_write_float(m_fluidSynth, samplesPerChannel, buffer, 0,
                          FLUID_AUDIO_CHANNELS_COUNT, buffer, 1,
                          FLUID_AUDIO_CHANNELS_COUNT);
}

} // namespace dgk
