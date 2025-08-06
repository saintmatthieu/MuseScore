/*
 * This file is part of Orchestrion.
 *
 * Copyright (C) 2024 Matthieu Hodgkinson
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "OrchestrionNotationInteraction.h"

namespace dgk
{
OrchestrionNotationInteraction::OrchestrionNotationInteraction(
    mu::notation::Notation *notation,
    mu::notation::INotationUndoStackPtr undoStack)
    : mu::notation::NotationInteraction(notation, std::move(undoStack))
{
}

void OrchestrionNotationInteraction::select(
    const std::vector<mu::engraving::EngravingItem *> &,
    mu::engraving::SelectType type, mu::engraving::staff_idx_t)
{
}

bool OrchestrionNotationInteraction::isDragStarted() const { return false; }

void OrchestrionNotationInteraction::startDrag(
    const std::vector<mu::engraving::EngravingItem *> &, const muse::PointF &,
    const IsDraggable &)
{
}

void OrchestrionNotationInteraction::drag(const muse::PointF &,
                                          const muse::PointF &,
                                          mu::notation::DragMode)
{
}

void OrchestrionNotationInteraction::endDrag() {}
} // namespace dgk