// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 Turan Furkan Topak <furkan1795@gmail.com>          *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#pragma once

#include <vector>

#include <Mod/Sketcher/App/GeoEnum.h>

#include "DrawSketchHandler.h"

namespace SketcherGui
{

class DrawSketchHandlerDragAutoConstraint final: public DrawSketchHandler
{
public:
    void mouseMove(SnapManager::SnapHandle /*snapHandle*/) override
    {}
    bool pressButton(Base::Vector2d /*pos*/) override
    {
        return false;
    }
    bool releaseButton(Base::Vector2d /*pos*/) override
    {
        return false;
    }

    std::string getToolName() const override
    {
        return "DSH_DragAutoConstraint";
    }

    QString getCrosshairCursorSVGName() const override
    {
        return QStringLiteral("Sketcher_Pointer_Create_Point");
    }

    void initDragging(const std::vector<Sketcher::GeoElementId>& dragged);
    void update(const std::vector<Sketcher::GeoElementId>& dragged, const Base::Vector2d& pos);
    void create(const std::vector<Sketcher::GeoElementId>& dragged);
    void clear();

private:
    bool canSuggestFor(const std::vector<Sketcher::GeoElementId>& dragged) const;
    void addAutoConstraint(
        Sketcher::ConstraintType type,
        int geoId,
        Sketcher::PointPos posId = Sketcher::PointPos::none
    );
    bool hasMoved(const Base::Vector2d& actualPos) const;
    Base::Vector2d getDirection(const Sketcher::GeoElementId& dragged, const Base::Vector2d& pos) const;
    bool isExistingConstraint(
        const Sketcher::GeoElementId& dragged,
        const AutoConstraint& constraint
    ) const;
    void removeInvalidConstraints(const Sketcher::GeoElementId& dragged);

private:
    std::vector<AutoConstraint> suggestedConstraints;
    Base::Vector2d startPos {0.0, 0.0};
};

}  // namespace SketcherGui
