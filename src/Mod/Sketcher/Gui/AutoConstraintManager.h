// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 Turan Furkan Topak                                 *
 *   <39885728+Reqrefusion@users.noreply.github.com>                       *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the          *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#pragma once

#include <Mod/Sketcher/App/Constraint.h>

namespace SketcherGui::AutoConstraintManager
{

/// Auto-constraint types controlled by the Auto Constraints toolbar.
enum class Mode
{
    Coincident,
    PointOnObject,
    Horizontal,
    Vertical,
    Parallel,
    Perpendicular,
    Tangent,
    Symmetric,
    Count
};

/// Return whether the given auto-constraint mode is enabled.
bool isModeActive(Mode mode);

/// Persist the state of the given auto-constraint mode.
void setModeActive(Mode mode, bool active);

/// Toggle and persist the state of the given auto-constraint mode.
void toggleMode(Mode mode);

/// Return whether a constraint type is enabled; unmanaged types remain enabled.
bool isConstraintActive(Sketcher::ConstraintType type);

}  // namespace SketcherGui::AutoConstraintManager
