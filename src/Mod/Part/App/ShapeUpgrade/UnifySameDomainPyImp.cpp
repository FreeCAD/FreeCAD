/***************************************************************************
 *   Copyright (c) 2020 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <memory>
# include <Standard_Failure.hxx>

// Needed for OCCT 7.5.2
#include <TopoDS_Edge.hxx>
#endif

#include <Base/PyWrapParseTupleAndKeywords.h>

#include "ShapeUpgrade/UnifySameDomainPy.h"
#include "ShapeUpgrade/UnifySameDomainPy.cpp"
#include "PartPyCXX.h"
#include "TopoShapePy.h"


using namespace Part;

PyObject *UnifySameDomainPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of UnifySameDomainPy
    return new UnifySameDomainPy(nullptr);
}

// constructor method
int UnifySameDomainPy::PyInit(PyObject* args, PyObject* kwds)
{
    PyObject *shape;
    PyObject *unifyEdges = Py_True; // NOLINT
    PyObject *unifyFaces = Py_True; // NOLINT
    PyObject *concatBSpl = Py_False; // NOLINT

    static const std::array<const char *, 5> keywords{"Shape", "UnifyEdges", "UnifyFaces", "ConcatBSplines", nullptr};
    if (!Base::Wrapped_ParseTupleAndKeywords(args, kwds, "O!|O!O!O!", keywords,
                                             &TopoShapePy::Type, &shape,
                                             &PyBool_Type, &unifyEdges,
                                             &PyBool_Type, &unifyFaces,
                                             &PyBool_Type, &concatBSpl)) {
        return -1;
    }

    try {
        TopoDS_Shape shp = static_cast<TopoShapePy*>(shape)->getTopoShapePtr()->getShape();
        std::unique_ptr<ShapeUpgrade_UnifySameDomain> ptr(new ShapeUpgrade_UnifySameDomain(shp,
                                                          Base::asBoolean(unifyEdges),
                                                          Base::asBoolean(unifyFaces),
                                                          Base::asBoolean(concatBSpl)));

        setTwinPointer(ptr.release());
        return 0;
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return -1;
    }
}

PyObject* UnifySameDomainPy::initialize(PyObject *args, PyObject* kwds)
{
    PyObject *shape;
    PyObject *unifyEdges = Py_True;
    PyObject *unifyFaces = Py_True;
    PyObject *concatBSpl = Py_False;

    static const std::array<const char *, 5> keywords{"Shape", "UnifyEdges", "UnifyFaces", "ConcatBSplines", nullptr};
    if (!Base::Wrapped_ParseTupleAndKeywords(args, kwds, "O!|O!O!O!", keywords,
                                             &TopoShapePy::Type, &shape,
                                             &PyBool_Type, &unifyEdges,
                                             &PyBool_Type, &unifyFaces,
                                             &PyBool_Type, &concatBSpl)) {
        return nullptr;
    }

    try {
        TopoDS_Shape shp = static_cast<TopoShapePy*>(shape)->getTopoShapePtr()->getShape();
        getShapeUpgrade_UnifySameDomainPtr()->Initialize(shp,
            Base::asBoolean(unifyEdges),
            Base::asBoolean(unifyFaces),
            Base::asBoolean(concatBSpl));

        Py_Return;
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return nullptr;
    }
}

// returns a string which represents the object e.g. when printed in python
std::string UnifySameDomainPy::representation() const
{
    return {"<ShapeUpgrade_UnifySameDomain object>"};
}

PyObject* UnifySameDomainPy::allowInternalEdges(PyObject *args)
{
    PyObject* allow;
    if (!PyArg_ParseTuple(args, "O!", &PyBool_Type, &allow))
        return nullptr;

    try {
        getShapeUpgrade_UnifySameDomainPtr()->AllowInternalEdges(Base::asBoolean(allow));
        Py_Return;
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* UnifySameDomainPy::keepShape(PyObject *args)
{
    PyObject* shape;
    if (!PyArg_ParseTuple(args, "O!", &TopoShapePy::Type, &shape))
        return nullptr;

    try {
        TopoDS_Shape shp = static_cast<TopoShapePy*>(shape)->getTopoShapePtr()->getShape();
        getShapeUpgrade_UnifySameDomainPtr()->KeepShape(shp);
        Py_Return;
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* UnifySameDomainPy::keepShapes(PyObject *args)
{
    PyObject* obj;
    if (!PyArg_ParseTuple(args, "O", &obj))
        return nullptr;

    try {
        TopTools_MapOfShape theShapes;
        Py::Sequence list(obj);
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            Py::TopoShape shp(*it);
            theShapes.Add(shp.extensionObject()->getTopoShapePtr()->getShape());
        }

        getShapeUpgrade_UnifySameDomainPtr()->KeepShapes(theShapes);
        Py_Return;
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* UnifySameDomainPy::setSafeInputMode(PyObject *args)
{
    PyObject* mode;
    if (!PyArg_ParseTuple(args, "O!", &PyBool_Type, &mode))
        return nullptr;

    try {
        getShapeUpgrade_UnifySameDomainPtr()->SetSafeInputMode(Base::asBoolean(mode));
        Py_Return;
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* UnifySameDomainPy::setLinearTolerance(PyObject *args)
{
    double linTol;
    if (!PyArg_ParseTuple(args, "d", &linTol))
        return nullptr;

    try {
        getShapeUpgrade_UnifySameDomainPtr()->SetLinearTolerance(linTol);
        Py_Return;
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* UnifySameDomainPy::setAngularTolerance(PyObject *args)
{
    double angTol;
    if (!PyArg_ParseTuple(args, "d", &angTol))
        return nullptr;

    try {
        getShapeUpgrade_UnifySameDomainPtr()->SetAngularTolerance(angTol);
        Py_Return;
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* UnifySameDomainPy::build(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    try {
        getShapeUpgrade_UnifySameDomainPtr()->Build();
        Py_Return;
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* UnifySameDomainPy::shape(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    try {
        TopoDS_Shape shape = getShapeUpgrade_UnifySameDomainPtr()->Shape();
        return new TopoShapePy(new TopoShape(shape));
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return nullptr;
    }
}

PyObject *UnifySameDomainPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int UnifySameDomainPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
