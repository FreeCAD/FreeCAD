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
# include <TopoDS.hxx>
# include <TopoDS_Edge.hxx>
# include <TopoDS_Face.hxx>
#endif

#include <Base/PyWrapParseTupleAndKeywords.h>
#include <Base/VectorPy.h>

#include "BRepFeat/MakePrismPy.h"
#include "BRepFeat/MakePrismPy.cpp"
#include "Geometry.h"
#include "TopoShapeEdgePy.h"
#include "TopoShapeFacePy.h"


using namespace Part;

PyObject *MakePrismPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of MakePrismPy
    return new MakePrismPy(nullptr);
}

// constructor method
int MakePrismPy::PyInit(PyObject* args, PyObject* kwds)
{
    PyObject* Sbase;
    PyObject* Pbase;
    PyObject* Skface;
    PyObject* Direction;
    int Fuse;
    PyObject* Modify;
    static const std::array<const char *, 7> keywords{"Sbase", "Pbase", "Skface", "Direction", "Fuse", "Modify",
                                                      nullptr};
    if (Base::Wrapped_ParseTupleAndKeywords(args, kwds, "O!O!O!O!iO!", keywords,
                                            &(TopoShapePy::Type), &Sbase,
                                            &(TopoShapePy::Type), &Pbase,
                                            &(TopoShapeFacePy::Type), &Skface,
                                            &(Base::VectorPy::Type), &Direction, &Fuse,
                                            &(PyBool_Type), &Modify)) {
        try {
            TopoDS_Shape sbase = static_cast<TopoShapePy*>(Sbase)->getTopoShapePtr()->getShape();
            TopoDS_Shape pbase = static_cast<TopoShapePy*>(Pbase)->getTopoShapePtr()->getShape();
            TopoDS_Face skface = TopoDS::Face(static_cast<TopoShapePy*>(Skface)->getTopoShapePtr()->getShape());
            Base::Vector3d dir = static_cast<Base::VectorPy*>(Direction)->value();
            std::unique_ptr<BRepFeat_MakePrism> ptr(new BRepFeat_MakePrism(sbase, pbase, skface, gp_Dir(dir.x, dir.y, dir.z), Fuse,
                                                                           Base::asBoolean(Modify)));

            setTwinPointer(ptr.release());
            return 0;
        }
        catch (const Standard_Failure& e) {
            PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
            return -1;
        }
    }

    PyErr_Clear();
    if (PyArg_ParseTuple(args, "")) {
        try {
            std::unique_ptr<BRepFeat_MakePrism> ptr(new BRepFeat_MakePrism());
            setTwinPointer(ptr.release());
            return 0;
        }
        catch (const Standard_Failure& e) {
            PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
            return -1;
        }
    }

    PyErr_SetString(PyExc_TypeError, "supported signatures:\n"
                    "MakePrism()\n"
                    "MakePrism(Sbase [shape], Pbase [shape], Skface [face], Direction [Vector], Fuse [int={0, 1}], Modify [bool])\n");
    return -1;
}

// returns a string which represents the object e.g. when printed in python
std::string MakePrismPy::representation() const
{
    return {"<BRepFeat_MakePrism object>"};
}

PyObject* MakePrismPy::init(PyObject *args,  PyObject* kwds)
{
    PyObject* Sbase;
    PyObject* Pbase;
    PyObject* Skface;
    PyObject* Direction;
    int Fuse;
    PyObject* Modify;
    static const std::array<const char *, 7> keywords{"Sbase", "Pbase", "Skface", "Direction", "Fuse", "Modify",
                                                      nullptr};
    if (!Base::Wrapped_ParseTupleAndKeywords(args, kwds, "O!O!O!O!iO!", keywords,
                                            &(TopoShapePy::Type), &Sbase,
                                            &(TopoShapePy::Type), &Pbase,
                                            &(TopoShapeFacePy::Type), &Skface,
                                            &(Base::VectorPy::Type), &Direction, &Fuse,
                                            &(PyBool_Type), &Modify)) {
        return nullptr;
    }


    try {
        TopoDS_Shape sbase = static_cast<TopoShapePy*>(Sbase)->getTopoShapePtr()->getShape();
        TopoDS_Shape pbase = static_cast<TopoShapePy*>(Pbase)->getTopoShapePtr()->getShape();
        TopoDS_Face skface = TopoDS::Face(static_cast<TopoShapePy*>(Skface)->getTopoShapePtr()->getShape());
        Base::Vector3d dir = static_cast<Base::VectorPy*>(Direction)->value();
        getBRepFeat_MakePrismPtr()->Init(sbase, pbase, skface, gp_Dir(dir.x, dir.y, dir.z), Fuse,
                                         Base::asBoolean(Modify));

        Py_Return;
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* MakePrismPy::add(PyObject *args,  PyObject* kwds)
{
    PyObject* Edge;
    PyObject* Face;
    static const std::array<const char *, 3> keywords{"Edge", "Face", nullptr};
    if (!Base::Wrapped_ParseTupleAndKeywords(args, kwds, "O!O!", keywords,
                                             &(TopoShapeEdgePy::Type), &Edge,
                                             &(TopoShapeFacePy::Type), &Face)) {
        return nullptr;
    }


    try {
        TopoDS_Edge edge = TopoDS::Edge(static_cast<TopoShapePy*>(Edge)->getTopoShapePtr()->getShape());
        TopoDS_Face face = TopoDS::Face(static_cast<TopoShapePy*>(Face)->getTopoShapePtr()->getShape());
        getBRepFeat_MakePrismPtr()->Add(edge, face);

        Py_Return;
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* MakePrismPy::perform(PyObject *args,  PyObject* kwds)
{
    PyObject* From;
    PyObject* Until;
    static const std::array<const char *, 3> keywords_fu{"From", "Until", nullptr};
    if (Base::Wrapped_ParseTupleAndKeywords(args, kwds, "O!O!", keywords_fu,
                                            &(TopoShapePy::Type), &From,
                                            &(TopoShapePy::Type), &Until)) {
        try {
            TopoDS_Shape from = static_cast<TopoShapePy*>(From)->getTopoShapePtr()->getShape();
            TopoDS_Shape until = static_cast<TopoShapePy*>(Until)->getTopoShapePtr()->getShape();
            getBRepFeat_MakePrismPtr()->Perform(from, until);
            Py_Return;
        }
        catch (const Standard_Failure& e) {
            PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
            return nullptr;
        }
    }

    PyErr_Clear();
    static const std::array<const char *, 2> keywords_u {"Until", nullptr};
    if (Base:: Wrapped_ParseTupleAndKeywords(args, kwds, "O!", keywords_u, &(TopoShapePy::Type), &Until)) {
        try {
            TopoDS_Shape until = static_cast<TopoShapePy*>(Until)->getTopoShapePtr()->getShape();
            getBRepFeat_MakePrismPtr()->Perform(until);
            Py_Return;
        }
        catch (const Standard_Failure& e) {
            PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
            return nullptr;
        }
    }

    PyErr_Clear();
    double length;
    static const std::array<const char *, 2> keywords_l {"Length", nullptr};
    if (Base::Wrapped_ParseTupleAndKeywords(args, kwds, "d", keywords_l, &length)) {
        try {
            getBRepFeat_MakePrismPtr()->Perform(length);
            Py_Return;
        }
        catch (const Standard_Failure& e) {
            PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
            return nullptr;
        }
    }

    PyErr_SetString(PyExc_TypeError, "supported signatures:\n"
                    "perform(From [shape], Until [shape])\n"
                    "perform(Until [shape])\n"
                    "perform(Length [float])\n");
    return nullptr;
}

PyObject* MakePrismPy::performUntilEnd(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    try {
        getBRepFeat_MakePrismPtr()->PerformUntilEnd();
        Py_Return;
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* MakePrismPy::performFromEnd(PyObject *args)
{
    PyObject* Until;
    if (!PyArg_ParseTuple(args, "O!", &(TopoShapePy::Type), &Until))
        return nullptr;

    try {
        TopoDS_Shape until = static_cast<TopoShapePy*>(Until)->getTopoShapePtr()->getShape();
        getBRepFeat_MakePrismPtr()->PerformFromEnd(until);
        Py_Return;
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* MakePrismPy::performThruAll(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    try {
        getBRepFeat_MakePrismPtr()->PerformThruAll();
        Py_Return;
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* MakePrismPy::performUntilHeight(PyObject *args)
{
    PyObject* Until;
    double length;
    if (!PyArg_ParseTuple(args, "O!d", &(TopoShapePy::Type), &Until, &length))
        return nullptr;

    try {
        TopoDS_Shape until = static_cast<TopoShapePy*>(Until)->getTopoShapePtr()->getShape();
        getBRepFeat_MakePrismPtr()->PerformUntilHeight(until, length);
        Py_Return;
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* MakePrismPy::curves(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    TColGeom_SequenceOfCurve S;
    getBRepFeat_MakePrismPtr()->Curves(S);

    Py::Tuple tuple(S.Length());
    for (int i = S.Lower(); i <= S.Upper(); ++i) {
        Handle(Geom_Curve) hC = S.Value(i);
        if (hC.IsNull())
            continue;
        std::unique_ptr<GeomCurve> gc(Part::makeFromCurve(hC));
        tuple.setItem(i, Py::asObject(gc->getPyObject()));
    }

    return Py::new_reference_to(tuple);
}

PyObject* MakePrismPy::barycCurve(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    Handle(Geom_Curve) hC = getBRepFeat_MakePrismPtr()->BarycCurve();
    if (hC.IsNull())
        Py_Return;
    std::unique_ptr<GeomCurve> gc(Part::makeFromCurve(hC));
    return gc->getPyObject();
}

PyObject* MakePrismPy::shape(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    try {
        TopoShape shape(getBRepFeat_MakePrismPtr()->Shape());
        return shape.getPyObject();
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return nullptr;
    }
}

PyObject *MakePrismPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int MakePrismPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
