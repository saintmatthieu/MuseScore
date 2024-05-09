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
#ifndef MU_NOTATION_NOTATIONMIDIINPUT_H
#define MU_NOTATION_NOTATIONMIDIINPUT_H

#include <QTimer>

#include "modularity/ioc.h"
#include "playback/iplaybackcontroller.h"
#include "inotationconfiguration.h"
#include "actions/iactionsdispatcher.h"

#include "../inotationmidiinput.h"
#include "igetscore.h"
#include "inotationinteraction.h"
#include "inotationundostack.h"
#include "dom/repeatList.h"

namespace mu::engraving {
class Score;
}

namespace mu::notation {

class SegmentIterator {
public:
  using RepeatSegmentVector = std::vector<engraving::RepeatSegment *>;

  SegmentIterator(const engraving::Score &score,
                  const RepeatSegmentVector &repeatList);

  Segment *next();
  //! If there are repeats, goes to the first iteration of that repeat.
  void goTo(Segment *segment);

private:
  static bool skipSegment(const Segment &segment,
                          const engraving::Score &score);

  void goToStart();
  Segment *nextRepeatSegment();
  Segment *nextMeasureSegment();

  const engraving::Score &m_score;
  const RepeatSegmentVector &m_repeatList;
  RepeatSegmentVector::const_iterator m_repeatSegmentIt;
  std::vector<const Measure *>::const_iterator m_measureIt;
  Segment *m_pSegment;
};

class NotationMidiInput : public INotationMidiInput
{
    INJECT(playback::IPlaybackController, playbackController)
    INJECT(muse::actions::IActionsDispatcher, dispatcher)
    INJECT(INotationConfiguration, configuration)

public:
    NotationMidiInput(IGetScore* getScore, INotationInteractionPtr notationInteraction, INotationUndoStackPtr undoStack);

    void onMidiEventReceived(const muse::midi::Event& event) override;
    async::Channel<std::vector<const Note*> > notesReceived() const override;

    void onRealtimeAdvance() override;

private:
    void goToElement(EngravingItem *el) override;

    void rewind() override;
    mu::engraving::Score* score() const;

    void doProcessEvents();
    Note* addNoteToScore(const muse::midi::Event& e);
    Note* makeNote(const muse::midi::Event& e);

    void enableMetronome();
    void disableMetronome();

    void runRealtime();
    void stopRealtime();

    void doRealtimeAdvance();
    void doExtendCurrentNote();

    NoteInputMethod noteInputMethod() const;
    bool isRealtime() const;
    bool isRealtimeAuto() const;
    bool isRealtimeManual() const;

    bool isNoteInputMode() const;

    IGetScore* m_getScore = nullptr;
    INotationInteractionPtr m_notationInteraction;
    INotationUndoStackPtr m_undoStack;
    async::Channel<std::vector<const Note*> > m_notesReceivedChannel;

    QTimer m_processTimer;
    std::vector<muse::midi::Event> m_eventsQueue;

    QTimer m_realtimeTimer;
    QTimer m_extendNoteTimer;
    bool m_allowRealtimeRests = false;

    bool m_shouldDisableMetronome = false;

    std::unique_ptr<SegmentIterator> m_segmentIterator;
};
}

#endif // MU_NOTATION_NOTATIONMIDIINPUT_H
