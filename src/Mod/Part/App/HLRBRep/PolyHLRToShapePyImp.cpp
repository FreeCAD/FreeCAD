/***************************************************************************
 *   Copyright (c) 2021 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include "HLRBRep/PolyHLRToShapePy.h"
#include "HLRBRep/PolyHLRToShapePy.cpp"
#include "HLRBRep/HLRBRep_PolyAlgoPy.h"
#include "TopoShapePy.h"


using namespace Part;

PyObject *PolyHLRToShapePy::PyMake(struct _typeobject *, PyObject *, PyObject *)
{
    // create a new instance of HLRBRep_AlgoPy
    return new PolyHLRToShapePy(new HLRBRep_PolyHLRToShape());
}

// constructor method
int PolyHLRToShapePy::PyInit(PyObject* args, PyObject* /*kwds*/)
{
    PyObject* algo = nullptr;
    if (!PyArg_ParseTuple(args, "|O!", &Part::HLRBRep_PolyAlgoPy::Type, &algo))
        return -1;

    if (algo) {
        HLRBRep_PolyAlgoPy* py = static_cast<HLRBRep_PolyAlgoPy*>(algo);
        getHLRBRep_PolyHLRToShapePtr()->Update(py->handle());
    }

    return 0;
}

// returns a string which represents the object e.g. when printed in python
std::string PolyHLRToShapePy::representation() const
{
    return {"<HLRBRep_PolyHLRToShape object>"};
}

PyObject* PolyHLRToShapePy::update(PyObject *args)
{
    PyObject* algo;
    if (!PyArg_ParseTuple(args, "O!", &Part::HLRBRep_PolyAlgoPy::Type, &algo))
        return nullptr;

    HLRBRep_PolyAlgoPy* py = static_cast<HLRBRep_PolyAlgoPy*>(algo);
    getHLRBRep_PolyHLRToShapePtr()->Update(py->handle());
    Py_Return;
}

PyObject* PolyHLRToShapePy::show(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    getHLRBRep_PolyHLRToShapePtr()->Show();
    Py_Return;
}

PyObject* PolyHLRToShapePy::hide(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    getHLRBRep_PolyHLRToShapePtr()->Hide();
    Py_Return;
}

PyObject* PolyHLRToShapePy::vCompound(PyObject *args)
{
    PyObject* shape = nullptr;
    if (!PyArg_ParseTuple(args, "|O!", &Part::TopoShapePy::Type, &shape))
        return nullptr;

    if (shape) {
        TopoDS_Shape input = static_cast<TopoShapePy*>(shape)->getTopoShapePtr()->getShape();
        TopoDS_Shape result = getHLRBRep_PolyHLRToShapePtr()->VCompound(input);
        return new TopoShapePy(new TopoShape(result));
    }
    else {
        TopoDS_Shape result = getHLRBRep_PolyHLRToShapePtr()->VCompound();
        return new TopoShapePy(new TopoShape(result));
    }
}

PyObject* PolyHLRToShapePy::Rg1LineVCompound(PyObject *args)
{
    PyObject* shape = nullptr;
    if (!PyArg_ParseTuple(args, "|O!", &Part::TopoShapePy::Type, &shape))
        return nullptr;

    if (shape) {
        TopoDS_Shape input = static_cast<TopoShapePy*>(shape)->getTopoShapePtr()->getShape();
        TopoDS_Shape result = getHLRBRep_PolyHLRToShapePtr()->Rg1LineVCompound(input);
        return new TopoShapePy(new TopoShape(result));
    }
    else {
        TopoDS_Shape result = getHLRBRep_PolyHLRToShapePtr()->Rg1LineVCompound();
        return new TopoShapePy(new TopoShape(result));
    }
}

PyObject* PolyHLRToShapePy::RgNLineVCompound(PyObject *args)
{
    PyObject* shape = nullptr;
    if (!PyArg_ParseTuple(args, "|O!", &Part::TopoShapePy::Type, &shape))
        return nullptr;

    if (shape) {
        TopoDS_Shape input = static_cast<TopoShapePy*>(shape)->getTopoShapePtr()->getShape();
        TopoDS_Shape result = getHLRBRep_PolyHLRToShapePtr()->RgNLineVCompound(input);
        return new TopoShapePy(new TopoShape(result));
    }
    else {
        TopoDS_Shape result = getHLRBRep_PolyHLRToShapePtr()->RgNLineVCompound();
        return new TopoShapePy(new TopoShape(result));
    }
}

PyObject* PolyHLRToShapePy::outLineVCompound(PyObject *args)
{
    PyObject* shape = nullptr;
    if (!PyArg_ParseTuple(args, "|O!", &Part::TopoShapePy::Type, &shape))
        return nullptr;

    if (shape) {
        TopoDS_Shape input = static_cast<TopoShapePy*>(shape)->getTopoShapePtr()->getShape();
        TopoDS_Shape result = getHLRBRep_PolyHLRToShapePtr()->OutLineVCompound(input);
        return new TopoShapePy(new TopoShape(result));
    }
    else {
        TopoDS_Shape result = getHLRBRep_PolyHLRToShapePtr()->OutLineVCompound();
        return new TopoShapePy(new TopoShape(result));
    }
}

PyObject* PolyHLRToShapePy::hCompound(PyObject *args)
{
    PyObject* shape = nullptr;
    if (!PyArg_ParseTuple(args, "|O!", &Part::TopoShapePy::Type, &shape))
        return nullptr;

    if (shape) {
        TopoDS_Shape input = static_cast<TopoShapePy*>(shape)->getTopoShapePtr()->getShape();
        TopoDS_Shape result = getHLRBRep_PolyHLRToShapePtr()->HCompound(input);
        return new TopoShapePy(new TopoShape(result));
    }
    else {
        TopoDS_Shape result = getHLRBRep_PolyHLRToShapePtr()->HCompound();
        return new TopoShapePy(new TopoShape(result));
    }
}

PyObject* PolyHLRToShapePy::Rg1LineHCompound(PyObject *args)
{
    PyObject* shape = nullptr;
    if (!PyArg_ParseTuple(args, "|O!", &Part::TopoShapePy::Type, &shape))
        return nullptr;

    if (shape) {
        TopoDS_Shape input = static_cast<TopoShapePy*>(shape)->getTopoShapePtr()->getShape();
        TopoDS_Shape result = getHLRBRep_PolyHLRToShapePtr()->Rg1LineHCompound(input);
        return new TopoShapePy(new TopoShape(result));
    }
    else {
        TopoDS_Shape result = getHLRBRep_PolyHLRToShapePtr()->Rg1LineHCompound();
        return new TopoShapePy(new TopoShape(result));
    }
}

PyObject* PolyHLRToShapePy::RgNLineHCompound(PyObject *args)
{
    PyObject* shape = nullptr;
    if (!PyArg_ParseTuple(args, "|O!", &Part::TopoShapePy::Type, &shape))
        return nullptr;

    if (shape) {
        TopoDS_Shape input = static_cast<TopoShapePy*>(shape)->getTopoShapePtr()->getShape();
        TopoDS_Shape result = getHLRBRep_PolyHLRToShapePtr()->RgNLineHCompound(input);
        return new TopoShapePy(new TopoShape(result));
    }
    else {
        TopoDS_Shape result = getHLRBRep_PolyHLRToShapePtr()->RgNLineHCompound();
        return new TopoShapePy(new TopoShape(result));
    }
}

PyObject* PolyHLRToShapePy::outLineHCompound(PyObject *args)
{
    PyObject* shape = nullptr;
    if (!PyArg_ParseTuple(args, "|O!", &Part::TopoShapePy::Type, &shape))
        return nullptr;

    if (shape) {
        TopoDS_Shape input = static_cast<TopoShapePy*>(shape)->getTopoShapePtr()->getShape();
        TopoDS_Shape result = getHLRBRep_PolyHLRToShapePtr()->OutLineHCompound(input);
        return new TopoShapePy(new TopoShape(result));
    }
    else {
        TopoDS_Shape result = getHLRBRep_PolyHLRToShapePtr()->OutLineHCompound();
        return new TopoShapePy(new TopoShape(result));
    }
}

PyObject *PolyHLRToShapePy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int PolyHLRToShapePy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
