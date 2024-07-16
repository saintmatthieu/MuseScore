#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <vector>

namespace dgk {

static constexpr auto numVoices = 4;

struct NoteEvent {
  enum class Type { noteOn, noteOff };

  Type type = Type::noteOn;
  int channel = 0;
  int pitch = 0;
  float velocity = 0.f;
};

class IChord;
class OrchestrionSequencer;

using ChordPtr = std::shared_ptr<IChord>;
using Staff = std::map<int /*voice*/, std::vector<ChordPtr>>;
using OrchestrionGetter = std::function<OrchestrionSequencer&()>;

class Finally {
public:
  Finally(std::function<void()> f) : m_f{std::move(f)} {}
  ~Finally() { m_f(); }

private:
  std::function<void()> m_f;
};
} // namespace dgk