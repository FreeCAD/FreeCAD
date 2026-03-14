/***************************************************************************
 *   Copyright (c) 2024 Martin Rodriguez Reboredo <yakoyoku@gmail.com>     *
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
#ifndef _PreComp_
#include <string>
#endif

// inclusion of the generated files (generated out of FontPy.xml)
#include "FontPy.h"

#include "FontPy.cpp"


using namespace Text;

// returns a string which represents the object e.g. when printed in python
std::string FontPy::representation() const
{
    return std::string("Text.Font('") + getFontPtr()->Name.getValue() + "')";
}

PyObject *FontPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    return new FontPy(new Font);
}

int FontPy::PyInit(PyObject* args, PyObject* /*kwds*/)
{
    PyObject* pFont;
    const char *Name = nullptr;
    if (!PyArg_ParseTuple(args, "O!|z", &(FontPy::Type), &pFont, &Name)) {
        return -1;
    }

    if (!Name || *Name == '\0') Name = "sans";

    FontPy* font = static_cast<FontPy*>(pFont);
    font->getFontPtr()->Name.setValue(Name);

    return 0;
}

Py::String FontPy::getName() const
{
    return {getFontPtr()->Name.getValue()};
}

void  FontPy::setName(Py::String arg)
{
    getFontPtr()->Name.setValue(arg);
}

Py::String FontPy::getSource() const
{
    return {getFontPtr()->Source.getValueAsString()};
}

void  FontPy::setSource(Py::String arg)
{
    getFontPtr()->Source.setValue(arg.as_std_string().c_str());
}

Py::String FontPy::getFile() const
{
    return {getFontPtr()->File.getValue()};
}

void  FontPy::setFile(Py::String arg)
{
    getFontPtr()->File.setValue(arg);
}

Py::String FontPy::getIncluded() const
{
    return {getFontPtr()->Included.getValue()};
}

void  FontPy::setIncluded(Py::String arg)
{
    getFontPtr()->Included.setValue(arg.as_std_string().c_str());
}

PyObject *FontPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int FontPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
