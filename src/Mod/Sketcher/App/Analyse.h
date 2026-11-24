// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2018 Abdullah Tahiri
// SPDX-FileCopyrightText: 2013 Werner Mayer
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#pragma once

#include <vector>

#include "Constraint.h"


namespace Sketcher
{

struct ConstraintIds
{
    Base::Vector3d v;
    int First;
    int Second;
    Sketcher::PointPos FirstPos;
    Sketcher::PointPos SecondPos;
    Sketcher::ConstraintType Type;
};

struct Constraint_Equal
{
    using argument_type = ConstraintIds;
    using result_type = bool;
    struct Sketcher::ConstraintIds c;
    explicit Constraint_Equal(const ConstraintIds& c)
        : c(c)
    {}
    bool operator()(const ConstraintIds& x) const
    {
        if (c.First == x.First && c.FirstPos == x.FirstPos && c.Second == x.Second
            && c.SecondPos == x.SecondPos) {
            return true;
        }
        if (c.Second == x.First && c.SecondPos == x.FirstPos && c.First == x.Second
            && c.FirstPos == x.SecondPos) {
            return true;
        }
        return false;
    }
};

}  // namespace Sketcher
