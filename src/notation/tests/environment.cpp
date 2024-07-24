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

#include "testing/environment.h"

#include "actions/actionsmodule.h"
#include "context/contextmodule.h"
#include "draw/drawmodule.h"
#include "engraving/engravingmodule.h"
#include "engraving/tests/utils/scorerw.h"
#include "multiinstances/multiinstancesmodule.h"
#include "notation/notationmodule.h"
#include "project/projectmodule.h"
#include "shortcuts/shortcutsmodule.h"
#include "stubs/midi/midistubmodule.h"
#include "stubs/mpe/mpestubmodule.h"
#include "stubs/playback/playbackstubmodule.h"
#include "stubs/workspace/workspacestubmodule.h"
#include "ui/uimodule.h"

#include "engraving/dom/instrtemplate.h"
#include "engraving/dom/mscore.h"

static mu::testing::SuiteEnvironment importexport_se(
{
    new muse::draw::DrawModule(),
    new mu::project::ProjectModule(),
    new muse::actions::ActionsModule(),
    new muse::mi::MultiInstancesModule(),
    new mu::context::ContextModule(),
    new muse::ui::UiModule(),
    new muse::shortcuts::ShortcutsModule(),
    new mu::playback::PlaybackModule(),
    new muse::workspace::WorkspaceModule(),
    new mu::engraving::EngravingModule(),
    new mu::notation::NotationModule(),
    new muse::midi::MidiModule(),
    new muse::mpe::MpeModule(),
},
    nullptr,
    []() {
    LOGI() << "notation tests suite post init";

    mu::engraving::ScoreRW::setRootPath(mu::String::fromUtf8(notation_tests_DATA_ROOT));

    mu::engraving::MScore::testMode = true;
    mu::engraving::MScore::testWriteStyleToScore = false;
    mu::engraving::MScore::noGui = true;

    mu::engraving::loadInstrumentTemplates(":/data/instruments.xml");
}
    );
