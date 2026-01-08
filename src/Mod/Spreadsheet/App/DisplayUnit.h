// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   © 2015 Eivind Kvedalen <eivind@kvedalen.name>                            *
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


#ifndef DISPLAYUNIT_H
#define DISPLAYUNIT_H

#include <Base/Unit.h>
#include <string>

namespace Spreadsheet
{

class DisplayUnit
{
public:
    std::string stringRep;
    Base::Unit unit;
    double scaler;

    explicit DisplayUnit(
        const std::string _stringRep = "",
        const Base::Unit _unit = Base::Unit(),
        double _scaler = 0.0
    )
        : stringRep(std::move(_stringRep))
        , unit(_unit)
        , scaler(_scaler)
    {}

    bool operator==(const DisplayUnit& c) const
    {
        return c.stringRep == stringRep && c.unit == unit && c.scaler == scaler;
    }

    bool operator!=(const DisplayUnit& c) const
    {
        return !operator==(c);
    }

    bool isEmpty() const
    {
        return stringRep.empty();
    }
};

}  // namespace Spreadsheet

#endif  // DISPLAYUNIT_H
