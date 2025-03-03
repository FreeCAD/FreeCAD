/***************************************************************************
 *   Copyright (c) 2010 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

// inclusion of the generated files (generated out of MaterialPy.xml)
#include "MaterialPy.h"

#include "MaterialPy.cpp"

#include <Base/PyWrapParseTupleAndKeywords.h>

using namespace App;

Base::Color MaterialPy::toColor(PyObject* value)
{
    Base::Color cCol;
    if (PyTuple_Check(value) && (PyTuple_Size(value) == 3 || PyTuple_Size(value) == 4)) {
        PyObject* item {};
        item = PyTuple_GetItem(value, 0);
        if (PyFloat_Check(item)) {
            cCol.r = (float)PyFloat_AsDouble(item);
            item = PyTuple_GetItem(value, 1);
            if (PyFloat_Check(item)) {
                cCol.g = (float)PyFloat_AsDouble(item);
            }
            else {
                throw Base::TypeError("Type in tuple must be consistent (float)");
            }
            item = PyTuple_GetItem(value, 2);
            if (PyFloat_Check(item)) {
                cCol.b = (float)PyFloat_AsDouble(item);
            }
            else {
                throw Base::TypeError("Type in tuple must be consistent (float)");
            }
            if (PyTuple_Size(value) == 4) {
                item = PyTuple_GetItem(value, 3);
                if (PyFloat_Check(item)) {
                    cCol.a = (float)PyFloat_AsDouble(item);
                }
                else {
                    throw Base::TypeError("Type in tuple must be consistent (float)");
                }
            }
        }
        else if (PyLong_Check(item)) {
            cCol.r = static_cast<float>(PyLong_AsLong(item)) / 255.0F;
            item = PyTuple_GetItem(value, 1);
            if (PyLong_Check(item)) {
                cCol.g = static_cast<float>(PyLong_AsLong(item)) / 255.0F;
            }
            else {
                throw Base::TypeError("Type in tuple must be consistent (integer)");
            }
            item = PyTuple_GetItem(value, 2);
            if (PyLong_Check(item)) {
                cCol.b = static_cast<float>(PyLong_AsLong(item)) / 255.0F;
            }
            else {
                throw Base::TypeError("Type in tuple must be consistent (integer)");
            }
            if (PyTuple_Size(value) == 4) {
                item = PyTuple_GetItem(value, 3);
                if (PyLong_Check(item)) {
                    cCol.a = static_cast<float>(PyLong_AsLong(item)) / 255.0F;
                }
                else {
                    throw Base::TypeError("Type in tuple must be consistent (integer)");
                }
            }
        }
        else {
            throw Base::TypeError("Type in tuple must be float or integer");
        }
    }
    else if (PyLong_Check(value)) {
        cCol.setPackedValue(PyLong_AsUnsignedLong(value));
    }
    else {
        std::string error =
            std::string("type must be integer or tuple of float or tuple integer, not ");
        error += value->ob_type->tp_name;
        throw Base::TypeError(error);
    }

    return cCol;
}

PyObject* MaterialPy::PyMake(struct _typeobject*, PyObject*, PyObject*)  // Python wrapper
{
    // create a new instance of MaterialPy and the Twin object
    return new MaterialPy(new Material);
}

// constructor method
int MaterialPy::PyInit(PyObject* args, PyObject* kwds)
{
    PyObject* diffuse = nullptr;
    PyObject* ambient = nullptr;
    PyObject* specular = nullptr;
    PyObject* emissive = nullptr;
    PyObject* shininess = nullptr;
    PyObject* transparency = nullptr;
    static const std::array<const char*, 7> kwds_colors {"DiffuseColor",
                                                         "AmbientColor",
                                                         "SpecularColor",
                                                         "EmissiveColor",
                                                         "Shininess",
                                                         "Transparency",
                                                         nullptr};

    if (!Base::Wrapped_ParseTupleAndKeywords(args,
                                             kwds,
                                             "|OOOOOO",
                                             kwds_colors,
                                             &diffuse,
                                             &ambient,
                                             &specular,
                                             &emissive,
                                             &shininess,
                                             &transparency)) {
        return -1;
    }

    try {
        if (diffuse) {
            setDiffuseColor(Py::Object(diffuse));
        }

        if (ambient) {
            setAmbientColor(Py::Object(ambient));
        }

        if (specular) {
            setSpecularColor(Py::Object(specular));
        }

        if (emissive) {
            setEmissiveColor(Py::Object(emissive));
        }

        if (shininess) {
            setShininess(Py::Float(shininess));
        }

        if (transparency) {
            setTransparency(Py::Float(transparency));
        }

        return 0;
    }
    catch (const Py::Exception&) {
        return -1;
    }
}

// returns a string which represents the object e.g. when printed in python
std::string MaterialPy::representation() const
{
    return {"<Material object>"};
}

PyObject* MaterialPy::set(PyObject* args)
{
    char* pstr {};
    if (!PyArg_ParseTuple(args, "s", &pstr)) {
        return nullptr;
    }

    getMaterialPtr()->set(pstr);

    Py_Return;
}

Py::Object MaterialPy::getAmbientColor() const
{
    Py::Tuple tuple(4);
    tuple.setItem(0, Py::Float(getMaterialPtr()->ambientColor.r));
    tuple.setItem(1, Py::Float(getMaterialPtr()->ambientColor.g));
    tuple.setItem(2, Py::Float(getMaterialPtr()->ambientColor.b));
    tuple.setItem(3, Py::Float(getMaterialPtr()->ambientColor.a));
    return tuple;
}

void MaterialPy::setAmbientColor(Py::Object arg)
{
    try {
        getMaterialPtr()->ambientColor = toColor(*arg);
    }
    catch (const Base::Exception& e) {
        e.setPyException();
        throw Py::Exception();
    }
}

Py::Object MaterialPy::getDiffuseColor() const
{
    Py::Tuple tuple(4);
    tuple.setItem(0, Py::Float(getMaterialPtr()->diffuseColor.r));
    tuple.setItem(1, Py::Float(getMaterialPtr()->diffuseColor.g));
    tuple.setItem(2, Py::Float(getMaterialPtr()->diffuseColor.b));
    tuple.setItem(3, Py::Float(getMaterialPtr()->diffuseColor.a));
    return tuple;
}

void MaterialPy::setDiffuseColor(Py::Object arg)
{
    try {
        getMaterialPtr()->diffuseColor = toColor(*arg);
    }
    catch (const Base::Exception& e) {
        e.setPyException();
        throw Py::Exception();
    }
}

Py::Object MaterialPy::getEmissiveColor() const
{
    Py::Tuple tuple(4);
    tuple.setItem(0, Py::Float(getMaterialPtr()->emissiveColor.r));
    tuple.setItem(1, Py::Float(getMaterialPtr()->emissiveColor.g));
    tuple.setItem(2, Py::Float(getMaterialPtr()->emissiveColor.b));
    tuple.setItem(3, Py::Float(getMaterialPtr()->emissiveColor.a));
    return tuple;
}

void MaterialPy::setEmissiveColor(Py::Object arg)
{
    try {
        getMaterialPtr()->emissiveColor = toColor(*arg);
    }
    catch (const Base::Exception& e) {
        e.setPyException();
        throw Py::Exception();
    }
}

Py::Object MaterialPy::getSpecularColor() const
{
    Py::Tuple tuple(4);
    tuple.setItem(0, Py::Float(getMaterialPtr()->specularColor.r));
    tuple.setItem(1, Py::Float(getMaterialPtr()->specularColor.g));
    tuple.setItem(2, Py::Float(getMaterialPtr()->specularColor.b));
    tuple.setItem(3, Py::Float(getMaterialPtr()->specularColor.a));
    return tuple;
}

void MaterialPy::setSpecularColor(Py::Object arg)
{
    try {
        getMaterialPtr()->specularColor = toColor(*arg);
    }
    catch (const Base::Exception& e) {
        e.setPyException();
        throw Py::Exception();
    }
}

Py::Float MaterialPy::getShininess() const
{
    return Py::Float(getMaterialPtr()->shininess);
}

void MaterialPy::setShininess(Py::Float arg)
{
    getMaterialPtr()->shininess = arg;
}

Py::Float MaterialPy::getTransparency() const
{
    return Py::Float(getMaterialPtr()->transparency);
}

void MaterialPy::setTransparency(Py::Float arg)
{
    getMaterialPtr()->transparency = arg;
}

PyObject* MaterialPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int MaterialPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
