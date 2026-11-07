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

#include <App/Document.h>
#include <Base/AxisPy.h>
#include <Base/QuantityPy.h>
#include <Base/Tools.h>
#include <Base/VectorPy.h>
#include <Mod/Part/App/Geometry.h>
#include <Mod/Part/App/LinePy.h>

// inclusion of the generated files (generated out of ShapeTextPy.xml)
#include "ShapeTextPy.h"

#include "ShapeTextPy.cpp"


using namespace Text;

// returns a string which represents the object e.g. when printed in python
std::string ShapeTextPy::representation() const
{
    return std::string("Text.ShapeText('") + getShapeTextPtr()->String.getValue() + "')";
}

PyObject *ShapeTextPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    return new ShapeTextPy(new ShapeText);
}

int ShapeTextPy::PyInit(PyObject* args, PyObject* /*kwds*/)
{
    PyObject* pText;
    const char *String = nullptr;
    const char *FontName = nullptr;
    double Size = 10.0;
    if (!PyArg_ParseTuple(args, "O!|zzd", &(ShapeTextPy::Type), &pText, &String, &FontName, &Size)) {
        return -1;
    }

    if (Size <= 0.0) {
        PyErr_SetString(PyExc_ValueError, "Invalid font size");
        return -1;
    }

    if (!String) String = "";
    if (!FontName || *FontName == '\0') FontName = "sans";

    ShapeTextPy* text = static_cast<ShapeTextPy*>(pText);
    text->getShapeTextPtr()->String.setValue(String);
    text->getShapeTextPtr()->FontName.setValue(FontName);
    text->getShapeTextPtr()->Size.setValue(Size);

    return 0;
}

Py::String ShapeTextPy::getString() const
{
    return {getShapeTextPtr()->String.getValue()};
}

void  ShapeTextPy::setString(Py::String arg)
{
    getShapeTextPtr()->String.setValue(arg);
}

Py::String ShapeTextPy::getFontSource() const
{
    return {getShapeTextPtr()->FontSource.getValueAsString()};
}

void  ShapeTextPy::setFontSource(Py::String arg)
{
    getShapeTextPtr()->FontSource.setValue(arg.as_std_string().c_str());
}

Py::String ShapeTextPy::getFontName() const
{
    return {getShapeTextPtr()->FontName.getValue()};
}

void  ShapeTextPy::setFontName(Py::String arg)
{
    getShapeTextPtr()->FontName.setValue(arg);
}

Py::String ShapeTextPy::getFontFile() const
{
    return {getShapeTextPtr()->FontFile.getValue()};
}

void  ShapeTextPy::setFontFile(Py::String arg)
{
    getShapeTextPtr()->FontFile.setValue(arg);
}

Py::Object ShapeTextPy::getFontObject() const
{
    return Py::Object(getShapeTextPtr()->FontObject.getPyObject(), true);
}

void  ShapeTextPy::setFontObject(Py::Object arg)
{
    // getShapeTextPtr()->FontObject.setValue(arg);
}

Py::Float ShapeTextPy::getSize() const
{
    return Py::Float(getShapeTextPtr()->Size.getValue());
}

void  ShapeTextPy::setSize(Py::Float arg)
{
    getShapeTextPtr()->Size.setValue(arg);
}

Py::String ShapeTextPy::getAspect() const
{
    return {getShapeTextPtr()->Aspect.getValueAsString()};
}

void  ShapeTextPy::setAspect(Py::String arg)
{
    getShapeTextPtr()->Aspect.setValue(arg.as_std_string().c_str());
}

Py::String ShapeTextPy::getJustification() const
{
    return {getShapeTextPtr()->Justification.getValueAsString()};
}

void  ShapeTextPy::setJustification(Py::String arg)
{
    getShapeTextPtr()->Justification.setValue(arg.as_std_string().c_str());
}

Py::String ShapeTextPy::getHeightReference() const
{
    return {getShapeTextPtr()->HeightReference.getValueAsString()};
}

void  ShapeTextPy::setHeightReference(Py::String arg)
{
    getShapeTextPtr()->HeightReference.setValue(arg.as_std_string().c_str());
}

Py::String ShapeTextPy::getDirection() const
{
    return {getShapeTextPtr()->Direction.getValueAsString()};
}

void  ShapeTextPy::setDirection(Py::String arg)
{
    getShapeTextPtr()->Direction.setValue(arg.as_std_string().c_str());
}

Py::Boolean ShapeTextPy::getKeepLeftMargin() const
{
    return {getShapeTextPtr()->KeepLeftMargin.getValue()};
}

void  ShapeTextPy::setKeepLeftMargin(Py::Boolean arg)
{
    getShapeTextPtr()->KeepLeftMargin.setValue(arg);
}

Py::Boolean ShapeTextPy::getScaleToSize() const
{
    return {getShapeTextPtr()->ScaleToSize.getValue()};
}

void  ShapeTextPy::setScaleToSize(Py::Boolean arg)
{
    getShapeTextPtr()->ScaleToSize.setValue(arg);
}

PyObject *ShapeTextPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int ShapeTextPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
