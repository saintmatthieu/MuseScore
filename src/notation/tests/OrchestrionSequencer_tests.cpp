/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore
 * Music Composition & Notation
 *
 * Copyright (C) 2024 MuseScore BVBA and others
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

#include "Orchestrion/OrchestrionSequencer.h"
#include "Orchestrion/OrchestrionTypes.h"
#include "engraving/dom/masterscore.h"
#include "engraving/dom/mscore.h"
#include "engraving/dom/repeatlist.h"
#include "io/ifilesystem.h"
#include "midi/midievent.h"
#include "notation/internal/orchestrion/OrchestrionSequencerFactory.h"
#include "project/iprojectcreator.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace mu;
using namespace mu::project;

namespace {
class ScoreReader {
public:
  INJECT(IProjectCreator, projectCreator);

  INotationProjectPtr load(const mu::String &filePath) {
    const auto creator = projectCreator();
    auto project = creator->newProject();
    assert(project);
    std::string format = io::suffix(filePath);
    Ret ret = project->load(filePath, "", false, format);
    assert(ret);
    return project;
  }
};
} // namespace

TEST(ChistophonePlayerTests, test) {
  mu::engraving::MScore::useRead302InTestMode = false;
  const mu::String TEST_SCORE_PATH(
      u"C:/Users/saint/git/github/musescore/MuseScore/src/notation/tests/data/"
      u"ChristophonePlayer_a.mscz");
  ScoreReader reader;
  const auto project = reader.load(TEST_SCORE_PATH);
  const auto score = project->masterNotation()->masterScore();
  const auto player = dgk::OrchestrionSequencerFactory::CreateSequencer(
      score->ntracks(), score->repeatList());
  ASSERT_TRUE(player != nullptr);
  struct Event {
    dgk::NoteEvent::Type type;
    uint8_t note;
  };
  const std::vector<Event> events{
      {dgk::NoteEvent::Type::noteOn, 60}, {dgk::NoteEvent::Type::noteOff, 60},
      {dgk::NoteEvent::Type::noteOn, 60}, {dgk::NoteEvent::Type::noteOff, 60},
      {dgk::NoteEvent::Type::noteOn, 60}, {dgk::NoteEvent::Type::noteOff, 60},
      {dgk::NoteEvent::Type::noteOn, 60}, {dgk::NoteEvent::Type::noteOff, 60},
      {dgk::NoteEvent::Type::noteOn, 60}, {dgk::NoteEvent::Type::noteOff, 60},
      {dgk::NoteEvent::Type::noteOn, 60}, {dgk::NoteEvent::Type::noteOff, 60},
      {dgk::NoteEvent::Type::noteOn, 60}, {dgk::NoteEvent::Type::noteOff, 60},
  };
  for (const auto &event : events) {
    dgk::NoteEvent noteEvent{event.type};
    noteEvent.pitch = event.note;
    noteEvent.velocity = 0.5f;
    player->OnInputEvent(noteEvent);
  }
}
