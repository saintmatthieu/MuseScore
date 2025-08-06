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
#pragma once

#include "notation/internal/notationinteraction.h"

namespace dgk
{
class OrchestrionNotationInteraction : public mu::notation::NotationInteraction
{
public:
  OrchestrionNotationInteraction(mu::notation::Notation *notation,
                                 mu::notation::INotationUndoStackPtr undoStack);

  void select(const std::vector<mu::engraving::EngravingItem *> &elements,
              mu::engraving::SelectType type,
              mu::engraving::staff_idx_t staffIndex) override;

  bool isDragStarted() const override;
  void startDrag(const std::vector<mu::engraving::EngravingItem *> &elems,
                 const muse::PointF &eoffset,
                 const IsDraggable &isDraggable) override;
  void drag(const muse::PointF &fromPos, const muse::PointF &toPos,
            mu::notation::DragMode mode) override;
  void endDrag() override;
};
} // namespace dgk