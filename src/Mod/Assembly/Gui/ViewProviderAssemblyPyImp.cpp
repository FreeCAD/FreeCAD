/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
 **************************************************************************/

#include "PreCompiled.h"

// inclusion of the generated files (generated out of ViewProviderAssemblyPy.xml)
#include "ViewProviderAssemblyPy.h"
#include "ViewProviderAssemblyPy.cpp"


using namespace Gui;

// returns a string which represents the object e.g. when printed in python
std::string ViewProviderAssemblyPy::representation() const
{
    std::stringstream str;
    str << "<Assembly View provider object at " << getViewProviderAssemblyPtr() << ">";

    return str.str();
}

Py::Boolean ViewProviderAssemblyPy::getEnableMovement() const
{
    return {getViewProviderAssemblyPtr()->getEnableMovement()};
}

void ViewProviderAssemblyPy::setEnableMovement(Py::Boolean arg)
{
    getViewProviderAssemblyPtr()->setEnableMovement(arg);
}

PyObject* ViewProviderAssemblyPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int ViewProviderAssemblyPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
