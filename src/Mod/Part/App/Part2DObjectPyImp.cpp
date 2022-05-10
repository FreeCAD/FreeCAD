/***************************************************************************
 *   Copyright (c) 2011 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include "Mod/Part/App/Part2DObject.h"
#include "OCCError.h"

// inclusion of the generated files (generated out of Part2DObjectPy.xml)
#include "Part2DObjectPy.h"
#include "Part2DObjectPy.cpp"

using namespace Part;

// returns a string which represents the object e.g. when printed in python
std::string Part2DObjectPy::representation(void) const
{
    return std::string("<Part::Part2DObject>");
}



PyObject *Part2DObjectPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int Part2DObjectPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}


