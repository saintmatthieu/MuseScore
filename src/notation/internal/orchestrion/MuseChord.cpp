#include "MuseChord.h"
#include "engraving/dom/chord.h"
#include "engraving/dom/mscore.h"
#include "engraving/dom/note.h"
#include "engraving/dom/rest.h"
#include "engraving/dom/score.h"
#include "engraving/dom/segment.h"
#include "engraving/dom/tie.h"
#include "notation/inotationinteraction.h"
#include <cassert>

namespace dgk {

namespace me = mu::engraving;

MuseChord::MuseChord(me::Score &score,
                     mu::notation::INotationInteraction &interaction,
                     const me::Segment &segment, size_t staffIdx, int voice)
    : m_score{score}, m_notationInteraction{interaction}, m_segment{segment},
      m_staffIdx{staffIdx}, m_voice{voice}, m_track{static_cast<int>(
                                                staffIdx * me::VOICES +
                                                voice)} {}

std::vector<me::Note *> MuseChord::GetNotes() const {
  if (const auto museChord =
          dynamic_cast<const me::Chord *>(m_segment.element(m_track)))
    return museChord->notes();
  return {};
}

std::vector<int> MuseChord::GetPitches() const {
  std::vector<int> chord;
  const auto notes = GetNotes();
  for (const auto note : notes)
    if (!note->tieBack())
      chord.push_back(note->pitch());
  return chord;
}

int MuseChord::GetTick() const { return m_segment.tick().ticks(); }

int MuseChord::GetEndTick() const {
  return GetTick() + m_segment.ticks().ticks();
}

void MuseChord::SetHighlight(bool value) {
  const auto notes = GetNotes();
  std::vector<me::EngravingItem *> items;
  std::for_each(notes.begin(), notes.end(), [&](me::Note *note) {
    while (note) {
      items.emplace_back(note);
      auto tie = note->tieFor();
      if (tie)
        items.emplace_back(tie);
      note = tie ? tie->endNote() : nullptr;
    }
  });

  if (notes.empty()) {
    // Get all consecutive rests, ignoring elements other than chords such as
    // bars, clefs, etc.
    auto segment = &m_segment;
    while (segment) {
      auto item = segment->element(m_track);
      if (dynamic_cast<me::Chord *>(item))
        break;
      if (const auto rest = dynamic_cast<me::Rest *>(item))
        items.emplace_back(rest);
      segment = segment->next();
    }
  }

  if (items.empty())
    return;

  if (value)
    m_score.select(items, me::SelectType::ADD);
  else
    std::for_each(items.begin(), items.end(),
                  [this](auto item) { m_score.deselect(item); });
  m_notationInteraction.selectionChanged().notify();
}

void MuseChord::ScrollToYou() const {
  m_notationInteraction.showItem(m_segment.element(m_track));
  m_notationInteraction.selectionChanged().notify();
}
} // namespace dgk