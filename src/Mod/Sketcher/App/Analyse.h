/***************************************************************************
 *   Copyright (c) 2018 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
 *   Copyright (c) 2013 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#ifndef SKETCHER_ANALYSE_H
#define SKETCHER_ANALYSE_H

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

#endif  // SKETCHER_ANALYSE_H
