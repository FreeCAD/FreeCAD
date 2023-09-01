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

#include <Base/PyWrapParseTupleAndKeywords.h>

#include "HLRBRep/HLRToShapePy.h"
#include "HLRBRep/HLRToShapePy.cpp"
#include "HLRBRep/HLRBRep_AlgoPy.h"
#include "TopoShapePy.h"


using namespace Part;

PyObject *HLRToShapePy::PyMake(struct _typeobject *, PyObject *, PyObject *)
{
    // create a new instance of HLRBRep_AlgoPy
    return new HLRToShapePy(nullptr);
}

// constructor method
int HLRToShapePy::PyInit(PyObject* args, PyObject* /*kwds*/)
{
    PyObject* algo;
    if (!PyArg_ParseTuple(args, "O!", &Part::HLRBRep_AlgoPy::Type, &algo))
        return -1;

    HLRBRep_AlgoPy* py = static_cast<HLRBRep_AlgoPy*>(algo);
    setTwinPointer(new HLRBRep_HLRToShape(py->handle()));

    return 0;
}

// returns a string which represents the object e.g. when printed in python
std::string HLRToShapePy::representation() const
{
    return {"<HLRBRep_HLRToShape object>"};
}

PyObject* HLRToShapePy::vCompound(PyObject *args)
{
    PyObject* shape = nullptr;
    if (!PyArg_ParseTuple(args, "|O!", &Part::TopoShapePy::Type, &shape))
        return nullptr;

    if (shape) {
        TopoDS_Shape input = static_cast<TopoShapePy*>(shape)->getTopoShapePtr()->getShape();
        TopoDS_Shape result = getHLRBRep_HLRToShapePtr()->VCompound(input);
        return new TopoShapePy(new TopoShape(result));
    }
    else {
        TopoDS_Shape result = getHLRBRep_HLRToShapePtr()->VCompound();
        return new TopoShapePy(new TopoShape(result));
    }
}

PyObject* HLRToShapePy::Rg1LineVCompound(PyObject *args)
{
    PyObject* shape = nullptr;
    if (!PyArg_ParseTuple(args, "|O!", &Part::TopoShapePy::Type, &shape))
        return nullptr;

    if (shape) {
        TopoDS_Shape input = static_cast<TopoShapePy*>(shape)->getTopoShapePtr()->getShape();
        TopoDS_Shape result = getHLRBRep_HLRToShapePtr()->Rg1LineVCompound(input);
        return new TopoShapePy(new TopoShape(result));
    }
    else {
        TopoDS_Shape result = getHLRBRep_HLRToShapePtr()->Rg1LineVCompound();
        return new TopoShapePy(new TopoShape(result));
    }
}

PyObject* HLRToShapePy::RgNLineVCompound(PyObject *args)
{
    PyObject* shape = nullptr;
    if (!PyArg_ParseTuple(args, "|O!", &Part::TopoShapePy::Type, &shape))
        return nullptr;

    if (shape) {
        TopoDS_Shape input = static_cast<TopoShapePy*>(shape)->getTopoShapePtr()->getShape();
        TopoDS_Shape result = getHLRBRep_HLRToShapePtr()->RgNLineVCompound(input);
        return new TopoShapePy(new TopoShape(result));
    }
    else {
        TopoDS_Shape result = getHLRBRep_HLRToShapePtr()->RgNLineVCompound();
        return new TopoShapePy(new TopoShape(result));
    }
}

PyObject* HLRToShapePy::outLineVCompound(PyObject *args)
{
    PyObject* shape = nullptr;
    if (!PyArg_ParseTuple(args, "|O!", &Part::TopoShapePy::Type, &shape))
        return nullptr;

    if (shape) {
        TopoDS_Shape input = static_cast<TopoShapePy*>(shape)->getTopoShapePtr()->getShape();
        TopoDS_Shape result = getHLRBRep_HLRToShapePtr()->OutLineVCompound(input);
        return new TopoShapePy(new TopoShape(result));
    }
    else {
        TopoDS_Shape result = getHLRBRep_HLRToShapePtr()->OutLineVCompound();
        return new TopoShapePy(new TopoShape(result));
    }
}

PyObject* HLRToShapePy::outLineVCompound3d(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    TopoDS_Shape result = getHLRBRep_HLRToShapePtr()->OutLineVCompound3d();
    return new TopoShapePy(new TopoShape(result));
}

PyObject* HLRToShapePy::isoLineVCompound(PyObject *args)
{
    PyObject* shape = nullptr;
    if (!PyArg_ParseTuple(args, "|O!", &Part::TopoShapePy::Type, &shape))
        return nullptr;

    if (shape) {
        TopoDS_Shape input = static_cast<TopoShapePy*>(shape)->getTopoShapePtr()->getShape();
        TopoDS_Shape result = getHLRBRep_HLRToShapePtr()->IsoLineVCompound(input);
        return new TopoShapePy(new TopoShape(result));
    }
    else {
        TopoDS_Shape result = getHLRBRep_HLRToShapePtr()->IsoLineVCompound();
        return new TopoShapePy(new TopoShape(result));
    }
}

PyObject* HLRToShapePy::hCompound(PyObject *args)
{
    PyObject* shape = nullptr;
    if (!PyArg_ParseTuple(args, "|O!", &Part::TopoShapePy::Type, &shape))
        return nullptr;

    if (shape) {
        TopoDS_Shape input = static_cast<TopoShapePy*>(shape)->getTopoShapePtr()->getShape();
        TopoDS_Shape result = getHLRBRep_HLRToShapePtr()->HCompound(input);
        return new TopoShapePy(new TopoShape(result));
    }
    else {
        TopoDS_Shape result = getHLRBRep_HLRToShapePtr()->HCompound();
        return new TopoShapePy(new TopoShape(result));
    }
}

PyObject* HLRToShapePy::Rg1LineHCompound(PyObject *args)
{
    PyObject* shape = nullptr;
    if (!PyArg_ParseTuple(args, "|O!", &Part::TopoShapePy::Type, &shape))
        return nullptr;

    if (shape) {
        TopoDS_Shape input = static_cast<TopoShapePy*>(shape)->getTopoShapePtr()->getShape();
        TopoDS_Shape result = getHLRBRep_HLRToShapePtr()->Rg1LineHCompound(input);
        return new TopoShapePy(new TopoShape(result));
    }
    else {
        TopoDS_Shape result = getHLRBRep_HLRToShapePtr()->Rg1LineHCompound();
        return new TopoShapePy(new TopoShape(result));
    }
}

PyObject* HLRToShapePy::RgNLineHCompound(PyObject *args)
{
    PyObject* shape = nullptr;
    if (!PyArg_ParseTuple(args, "|O!", &Part::TopoShapePy::Type, &shape))
        return nullptr;

    if (shape) {
        TopoDS_Shape input = static_cast<TopoShapePy*>(shape)->getTopoShapePtr()->getShape();
        TopoDS_Shape result = getHLRBRep_HLRToShapePtr()->RgNLineHCompound(input);
        return new TopoShapePy(new TopoShape(result));
    }
    else {
        TopoDS_Shape result = getHLRBRep_HLRToShapePtr()->RgNLineHCompound();
        return new TopoShapePy(new TopoShape(result));
    }
}

PyObject* HLRToShapePy::outLineHCompound(PyObject *args)
{
    PyObject* shape = nullptr;
    if (!PyArg_ParseTuple(args, "|O!", &Part::TopoShapePy::Type, &shape))
        return nullptr;

    if (shape) {
        TopoDS_Shape input = static_cast<TopoShapePy*>(shape)->getTopoShapePtr()->getShape();
        TopoDS_Shape result = getHLRBRep_HLRToShapePtr()->OutLineHCompound(input);
        return new TopoShapePy(new TopoShape(result));
    }
    else {
        TopoDS_Shape result = getHLRBRep_HLRToShapePtr()->OutLineHCompound();
        return new TopoShapePy(new TopoShape(result));
    }
}

PyObject* HLRToShapePy::isoLineHCompound(PyObject *args)
{
    PyObject* shape = nullptr;
    if (!PyArg_ParseTuple(args, "|O!", &Part::TopoShapePy::Type, &shape))
        return nullptr;

    if (shape) {
        TopoDS_Shape input = static_cast<TopoShapePy*>(shape)->getTopoShapePtr()->getShape();
        TopoDS_Shape result = getHLRBRep_HLRToShapePtr()->IsoLineHCompound(input);
        return new TopoShapePy(new TopoShape(result));
    }
    else {
        TopoDS_Shape result = getHLRBRep_HLRToShapePtr()->IsoLineHCompound();
        return new TopoShapePy(new TopoShape(result));
    }
}

PyObject* HLRToShapePy::compoundOfEdges(PyObject *args, PyObject *kwds)
{
    int type;
    PyObject* visible = nullptr;
    PyObject* in3d = nullptr;
    PyObject* shape = nullptr;

    static const std::array<const char *, 5> keywords {"Type", "Visible", "In3D", "Shape", nullptr};
    if (!Base::Wrapped_ParseTupleAndKeywords(args, kwds, "iO!O!|O!", keywords,
                                             &type,
                                             &PyBool_Type, &visible,
                                             &PyBool_Type, &in3d,
                                             &Part::TopoShapePy::Type, &shape)) {
        return nullptr;
    }

    if (shape) {
        TopoDS_Shape input = static_cast<TopoShapePy*>(shape)->getTopoShapePtr()->getShape();
        TopoDS_Shape result = getHLRBRep_HLRToShapePtr()->CompoundOfEdges(input, static_cast<HLRBRep_TypeOfResultingEdge>(type),
                                                                          Base::asBoolean(visible), Base::asBoolean(in3d));
        return new TopoShapePy(new TopoShape(result));
    }
    else {
        TopoDS_Shape result = getHLRBRep_HLRToShapePtr()->CompoundOfEdges(static_cast<HLRBRep_TypeOfResultingEdge>(type),
                                                                          Base::asBoolean(visible), Base::asBoolean(in3d));
        return new TopoShapePy(new TopoShape(result));
    }
}

PyObject *HLRToShapePy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int HLRToShapePy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
