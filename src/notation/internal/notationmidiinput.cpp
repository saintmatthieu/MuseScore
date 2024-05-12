/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore
 * Music Composition & Notation
 *
 * Copyright (C) 2021 MuseScore BVBA and others
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "notationmidiinput.h"
#include "ChordRestIterator.h"

#include <QGuiApplication>

#include "engraving/dom/masterscore.h"
#include "engraving/dom/segment.h"
#include "engraving/dom/tie.h"
#include "engraving/dom/score.h"
#include "engraving/dom/note.h"
#include "engraving/dom/rest.h"
#include "engraving/dom/factory.h"
#include "engraving/types/fraction.h"

#include "notationtypes.h"

#include "defer.h"
#include "log.h"

#include <unordered_map>

using namespace mu::notation;

static constexpr int PROCESS_INTERVAL = 20;

NotationMidiInput::NotationMidiInput(IGetScore* getScore, INotationInteractionPtr notationInteraction, INotationUndoStackPtr undoStack)
    : m_getScore(getScore), m_notationInteraction(notationInteraction), m_undoStack(undoStack)
{
    QObject::connect(&m_processTimer, &QTimer::timeout, [this]() { doProcessEvents(); });

    m_realtimeTimer.setTimerType(Qt::PreciseTimer);
    QObject::connect(&m_realtimeTimer, &QTimer::timeout, [this]() { doRealtimeAdvance(); });

    m_extendNoteTimer.setTimerType(Qt::PreciseTimer);
    m_extendNoteTimer.setSingleShot(true);
    QObject::connect(&m_extendNoteTimer, &QTimer::timeout, [this]() { doExtendCurrentNote(); });
}

void NotationMidiInput::onMidiEventReceived(const muse::midi::Event& event)
{
    if (event.isChannelVoice20()) {
        auto events = event.toMIDI10();
        for (auto& midi10event : events) {
            onMidiEventReceived(midi10event);
        }

        return;
    }

    if (event.opcode() == muse::midi::Event::Opcode::NoteOn) {
        m_eventsQueue.push_back(event);

        if (!m_processTimer.isActive()) {
            m_processTimer.start(PROCESS_INTERVAL);
        }
    }
}

mu::async::Channel<std::vector<const Note*> > NotationMidiInput::notesReceived() const
{
    return m_notesReceivedChannel;
}

void NotationMidiInput::onRealtimeAdvance()
{
    if (!isNoteInputMode()) {
        return;
    }

    if (isRealtimeManual()) {
        m_allowRealtimeRests = true;
        enableMetronome();
        doRealtimeAdvance();
    } else if (isRealtimeAuto()) {
        if (m_realtimeTimer.isActive()) {
            stopRealtime();
            disableMetronome();
        } else {
            m_allowRealtimeRests = true;
            enableMetronome();
            runRealtime();
        }
    }
}

void NotationMidiInput::rewind() {
  for (auto &it : m_chordRestIterators)
    it.reset();
  playbackController()->seek(int64_t{0});
  score()->deselectAll();
}

void NotationMidiInput::goToElement(EngravingItem *el) {
  auto note = dynamic_cast<Note*>(el);
  if (!note) {
    return;
  }
  rewind();
  if (!m_chordRestIterators[0]) {
    auto rightHand = true;
    for (auto &it : m_chordRestIterators) {
      it.reset(
          new ChordRestIterator(*score(), score()->repeatList(), rightHand));
      it->goTo(note->chord()->segment());
      rightHand = false;
    }
  }
}

mu::engraving::Score* NotationMidiInput::score() const
{
    IF_ASSERT_FAILED(m_getScore) {
        return nullptr;
    }

    return m_getScore->score();
}

void NotationMidiInput::doProcessEvents()
{
    if (m_eventsQueue.empty()) {
        m_processTimer.stop();
        return;
    }

    std::vector<Note*> notes;

    NotePerformanceAttributeMap attributeMap;

    for (size_t i = 0; i < m_eventsQueue.size(); ++i) {
        const muse::midi::Event& event = m_eventsQueue.at(i);

        if (event.opcode() != muse::midi::Event::Opcode::NoteOn || event.velocity() == 0)
            continue;

        auto attributes = std::make_shared<PerformanceAttributes>(PerformanceAttributes{ event.velocity() / 128., event.pitchNote() >= 60 });

        if (!m_chordRestIterators[0]) {
          auto rightHand = true;
          for (auto &it : m_chordRestIterators) {
            it.reset(new ChordRestIterator(*score(), score()->repeatList(),
                                           rightHand));
            rightHand = false;
          }
        }

        for (auto &it : m_chordRestIterators)
          if (auto segment = it->next(event.pitchNote())) {
            const auto chords =
                ChordRestIterator::getChords(*segment, *score(), it->isRightHand());
            std::for_each(chords.begin(), chords.end(), [&](Chord *chord) {
              const auto chordNotes = ChordRestIterator::getUntiedNotes(*chord);
              std::for_each(
                  chordNotes.begin(), chordNotes.end(),
                [&](Note* note) { attributeMap.insert({ note, attributes }); });
              notes.insert(notes.end(), chordNotes.begin(), chordNotes.end());
            });
          }

        bool chord = i != 0;
        bool noteOn = event.opcode() == muse::midi::Event::Opcode::NoteOn;
        if (!chord && noteOn && !m_realtimeTimer.isActive() && isRealtimeAuto()) {
            m_extendNoteTimer.start(configuration()->delayBetweenNotesInRealTimeModeMilliseconds());
            enableMetronome();
            doRealtimeAdvance();
        }
    }

    if (!notes.empty()) {
        std::vector<EngravingItem*> notesItems;
        for (Note* note : notes) {
            notesItems.push_back(note);
        }

        playbackController()->playElements(
            {notesItems.begin(), notesItems.end()}, std::move(attributeMap));
        m_notesReceivedChannel.send({notes.begin(), notes.end()});

        auto pScore = score();
        pScore->deselectAll();
        pScore->select(notesItems, engraving::SelectType::ADD);
        m_notationInteraction->showItem(notesItems[0]);
    }

    m_eventsQueue.clear();
    m_processTimer.stop();
}

Note* NotationMidiInput::addNoteToScore(const muse::midi::Event& e)
{
    mu::engraving::Score* sc = score();
    if (!sc) {
        return nullptr;
    }

    mu::engraving::MidiInputEvent inputEv;
    inputEv.pitch = e.note();
    inputEv.velocity = e.velocity();

    sc->activeMidiPitches().remove_if([&inputEv](const mu::engraving::MidiInputEvent& val) {
        return inputEv.pitch == val.pitch;
    });

    const mu::engraving::InputState& is = sc->inputState();
    if (!is.noteEntryMode()) {
        return nullptr;
    }

    DEFER {
        m_undoStack->commitChanges();
    };

    m_undoStack->prepareChanges();

    if (e.opcode() == muse::midi::Event::Opcode::NoteOff) {
        if (isRealtime()) {
            const Chord* chord = is.cr()->isChord() ? engraving::toChord(is.cr()) : nullptr;
            if (chord) {
                Note* n = chord->findNote(inputEv.pitch);
                if (n) {
                    sc->deleteItem(n->tieBack());
                    sc->deleteItem(n);
                }
            }
        }

        return nullptr;
    }

    if (sc->activeMidiPitches().empty()) {
        inputEv.chord = false;
    } else {
        inputEv.chord = true;
    }

    // holding shift while inputting midi will add the new pitch to the prior existing chord
    if (QGuiApplication::keyboardModifiers() & Qt::ShiftModifier) {
        mu::engraving::EngravingItem* cr = is.lastSegment()->element(is.track());
        if (cr && cr->isChord()) {
            inputEv.chord = true;
        }
    }

    mu::engraving::Note* note = sc->addMidiPitch(inputEv.pitch, inputEv.chord);

    sc->activeMidiPitches().push_back(inputEv);

    m_notationInteraction->showItem(is.cr());

    return note;
}

Note* NotationMidiInput::makeNote(const muse::midi::Event& e)
{
    if (e.opcode() == muse::midi::Event::Opcode::NoteOff || e.velocity() == 0) {
        return nullptr;
    }

    mu::engraving::Score* score = this->score();
    if (!score) {
        return nullptr;
    }

    if (score->selection().isNone()) {
        return nullptr;
    }

    const mu::engraving::InputState& inputState = score->inputState();
    if (!inputState.cr()) {
        return nullptr;
    }

    Chord* chord = engraving::Factory::createChord(inputState.lastSegment());
    chord->setParent(inputState.lastSegment());

    Note* note = engraving::Factory::createNote(chord);
    note->setParent(chord);
    note->setStaffIdx(engraving::track2staff(inputState.cr()->track()));

    engraving::NoteVal nval = score->noteVal(e.note());
    note->setNval(nval);

    return note;
}

void NotationMidiInput::enableMetronome()
{
    bool metronomeEnabled = configuration()->isMetronomeEnabled();
    if (metronomeEnabled) {
        return;
    }

    dispatcher()->dispatch("metronome");
    m_shouldDisableMetronome = true;
}

void NotationMidiInput::disableMetronome()
{
    if (!m_shouldDisableMetronome) {
        return;
    }

    dispatcher()->dispatch("metronome");

    m_shouldDisableMetronome = false;
}

void NotationMidiInput::runRealtime()
{
    m_realtimeTimer.start(configuration()->delayBetweenNotesInRealTimeModeMilliseconds());
}

void NotationMidiInput::stopRealtime()
{
    m_realtimeTimer.stop();
}

void NotationMidiInput::doRealtimeAdvance()
{
    if (!isRealtime() || !isNoteInputMode() || (!m_allowRealtimeRests && m_getScore->score()->activeMidiPitches().empty())) {
        if (m_realtimeTimer.isActive()) {
            stopRealtime();
        }

        disableMetronome();

        m_allowRealtimeRests = true;
        return;
    }

    const mu::engraving::InputState& is = m_getScore->score()->inputState();
    playbackController()->playMetronome(is.tick().ticks());

    QTimer::singleShot(100, Qt::PreciseTimer, [this]() {
        m_undoStack->prepareChanges();
        m_getScore->score()->realtimeAdvance();
        m_undoStack->commitChanges();
    });

    if (isRealtimeManual()) {
        int metronomeDuration = 500;
        QTimer::singleShot(metronomeDuration, Qt::PreciseTimer, [this]() {
            disableMetronome();
        });
    }
}

void NotationMidiInput::doExtendCurrentNote()
{
    if (!isNoteInputMode() || m_realtimeTimer.isActive()) {
        return;
    }

    m_allowRealtimeRests = false;
    runRealtime();
    doRealtimeAdvance();
}

NoteInputMethod NotationMidiInput::noteInputMethod() const
{
    return m_notationInteraction->noteInput()->state().method;
}

bool NotationMidiInput::isRealtime() const
{
    return isRealtimeAuto() || isRealtimeManual();
}

bool NotationMidiInput::isRealtimeAuto() const
{
    return noteInputMethod() == NoteInputMethod::REALTIME_AUTO;
}

bool NotationMidiInput::isRealtimeManual() const
{
    return noteInputMethod() == NoteInputMethod::REALTIME_MANUAL;
}

bool NotationMidiInput::isNoteInputMode() const
{
    return m_notationInteraction->noteInput()->isNoteInputMode();
}
