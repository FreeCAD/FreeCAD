/***************************************************************************
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

#include "PreCompiled.h"

// inclusion of the generated files (generated out of NavigationStylePy.pyi)
#include "Navigation/NavigationStylePy.h"
#include "Navigation/NavigationStylePy.cpp"


using namespace Gui;

/** @class NavigationStylePy
 * The NavigationStyle Python class provides additional methods for manipulation of
 * navigation style objects.
 * @see NavigationStyle
 */

// returns a string which represent the object e.g. when printed in python
std::string NavigationStylePy::representation() const
{
    return {"<NavigationStyle object>"};
}

PyObject* NavigationStylePy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int NavigationStylePy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
