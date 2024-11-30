/***************************************************************************
 *   Copyright (c) 2016 WandererFan <wandererfan@gmail.com>                *
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

#include <Base/FileInfo.h>
#include <Base/Stream.h>

#include "DrawViewSymbol.h"
#include "DrawView.h"
// inclusion of the generated files
#include <Mod/TechDraw/App/DrawViewPy.h>
#include <Mod/TechDraw/App/DrawViewSymbolPy.h>
#include <Mod/TechDraw/App/DrawViewSymbolPy.cpp>


using namespace TechDraw;

// returns a string which represents the object e.g. when printed in python
std::string DrawViewSymbolPy::representation() const
{
    return std::string("<DrawViewSymbol object>");
}

PyObject* DrawViewSymbolPy::dumpSymbol(PyObject *args)
{
    const char* fileSpec;
    if (!PyArg_ParseTuple(args, "s", &fileSpec)) {
        return nullptr;
    }

    std::string symbolRepr = getDrawViewSymbolPtr()->Symbol.getValue();
    Base::FileInfo fi(fileSpec);
    Base::ofstream outfile;
    outfile.open(fi);
    outfile.write (symbolRepr.c_str(), symbolRepr.size());
    outfile.close();
    if (outfile.good()) {
        outfile.close();
    } else {
        std::string error = std::string("Can't write ");
        error += fileSpec;
        PyErr_SetString(PyExc_RuntimeError, error.c_str());
        return nullptr;
    }

    Py_Return;
}

PyObject *DrawViewSymbolPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int DrawViewSymbolPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
