/***************************************************************************
 *   Copyright (c) 2012 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <BRepOffsetAPI_MakeFilling.hxx>
# include <GeomAbs_Shape.hxx>
# include <gp_Pnt.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Face.hxx>
#endif

#include <Base/PyWrapParseTupleAndKeywords.h>
#include <Base/VectorPy.h>

#include "BRepOffsetAPI_MakeFillingPy.h"
#include "BRepOffsetAPI_MakeFillingPy.cpp"
#include "TopoShapeEdgePy.h"
#include "TopoShapeFacePy.h"


using namespace Part;
/*!
 * \brief BRepOffsetAPI_MakeFillingPy::PyMake
 * \code
v1=App.Vector(0,0,0)
v2=App.Vector(10,0,0)
v3=App.Vector(10,10,3)
v4=App.Vector(0,10,0)
v5=App.Vector(5,5,5)

l1=Part.makeLine(v1, v2)
l2=Part.makeLine(v2, v3)
l3=Part.makeLine(v3, v4)
l4=Part.makeLine(v4, v1)

bp=Part.BRepOffsetAPI.MakeFilling()
bp.add(l1, 0, True)
bp.add(l2, 0, True)
bp.add(l3, 0, True)
bp.add(l4, 0, True)
bp.add(v5)
bp.build()
s=bp.shape()
Part.show(s)
Part.show(l1)
Part.show(l2)
Part.show(l3)
Part.show(l4)

bp.surfInit()
 * \endcode
 */

PyObject *BRepOffsetAPI_MakeFillingPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of BRepOffsetAPI_MakeFillingPy
    return new BRepOffsetAPI_MakeFillingPy(nullptr);
}

// constructor method
int BRepOffsetAPI_MakeFillingPy::PyInit(PyObject* args, PyObject* kwds)
{
    int degree = 3;
    int nbPtsOnCur = 15;
    int nbIter = 2;
    int maxDeg = 8;
    int maxSegments = 9;
    double tol2d = 0.00001;
    double tol3d = 0.0001;
    double tolAng = 0.01;
    double tolCurv = 0.1;
    PyObject* anisotropy = Py_False;

    static const std::array<const char *, 11> keywords{"Degree", "NbPtsOnCur", "NbIter", "MaxDegree", "MaxSegments",
                                                       "Tol2d", "Tol3d", "TolAng", "TolCurv", "Anisotropy", nullptr};
    if (!Base::Wrapped_ParseTupleAndKeywords(args, kwds, "|iiiiiddddO!", keywords,
                                             &degree, &nbPtsOnCur, &nbIter, &maxDeg, &maxSegments,
                                             &tol2d, &tol3d, &tolAng, &tolCurv, &PyBool_Type, &anisotropy)) {
        return -1;
    }

    try {
        std::unique_ptr<BRepOffsetAPI_MakeFilling> ptr(new BRepOffsetAPI_MakeFilling(degree, nbPtsOnCur, nbIter,
                                                          Base::asBoolean(anisotropy),
                                                          tol2d, tol3d, tolAng, tolCurv, maxDeg, maxSegments));


        setTwinPointer(ptr.release());
        return 0;
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return -1;
    }
}

// returns a string which represents the object e.g. when printed in python
std::string BRepOffsetAPI_MakeFillingPy::representation() const
{
    return {"<BRepOffsetAPI_MakeFilling object>"};
}

PyObject* BRepOffsetAPI_MakeFillingPy::setConstrParam(PyObject *args, PyObject *kwds)
{
    double tol2d = 0.00001;
    double tol3d = 0.0001;
    double tolAng = 0.01;
    double tolCurv = 0.1;

    static const std::array<const char *, 5> keywords {"Tol2d", "Tol3d", "TolAng", "TolCurv", nullptr};
    if (!Base::Wrapped_ParseTupleAndKeywords(args, kwds, "|dddd", keywords,
                                            &tol2d, &tol3d, &tolAng, &tolCurv)) {
        return nullptr;
    }

    try {
        getBRepOffsetAPI_MakeFillingPtr()->SetConstrParam(tol2d, tol3d, tolAng, tolCurv);
        Py_Return;
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BRepOffsetAPI_MakeFillingPy::setResolParam(PyObject *args, PyObject *kwds)
{
    int degree = 3;
    int nbPtsOnCur = 15;
    int nbIter = 2;
    PyObject* anisotropy = Py_False;

    static const std::array<const char *, 5> keywords {"Degree", "NbPtsOnCur", "NbIter", "Anisotropy", nullptr};
    if (!Base::Wrapped_ParseTupleAndKeywords(args, kwds, "|iiiO!", keywords,
                                             &degree, &nbPtsOnCur, &nbIter, &PyBool_Type, &anisotropy)) {
        return nullptr;
    }

    try {
        getBRepOffsetAPI_MakeFillingPtr()->SetResolParam(degree, nbPtsOnCur, nbIter,
                                                         Base::asBoolean(anisotropy));
        Py_Return;
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BRepOffsetAPI_MakeFillingPy::setApproxParam(PyObject *args, PyObject *kwds)
{
    int maxDeg = 8;
    int maxSegments = 9;

    static const std::array<const char *, 3> keywords {"MaxDegree", "MaxSegments", nullptr};
    if (!Base::Wrapped_ParseTupleAndKeywords(args, kwds, "|ii", keywords,
                                             &maxDeg, &maxSegments)) {
        return nullptr;
    }

    try {
        getBRepOffsetAPI_MakeFillingPtr()->SetApproxParam(maxDeg, maxSegments);
        Py_Return;
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BRepOffsetAPI_MakeFillingPy::loadInitSurface(PyObject *args)
{
    PyObject* shape;
    if (!PyArg_ParseTuple(args, "O!", &(TopoShapeFacePy::Type), &shape))
        return nullptr;

    TopoDS_Face face = TopoDS::Face(static_cast<TopoShapeFacePy*>(shape)->getTopoShapePtr()->getShape());
    if (face.IsNull()) {
        PyErr_SetString(PyExc_ReferenceError, "No valid face");
        return nullptr;
    }

    try {
        getBRepOffsetAPI_MakeFillingPtr()->LoadInitSurface(face);
        Py_Return;
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BRepOffsetAPI_MakeFillingPy::add(PyObject *args, PyObject *kwds)
{
    // 1st
    PyObject* pnt;
    static const std::array<const char *, 2> keywords_pnt {"Point", nullptr};
    if (Base::Wrapped_ParseTupleAndKeywords(args, kwds, "O!", keywords_pnt,
                                            &Base::VectorPy::Type, &pnt)) {
        try {
            Base::Vector3d vec = static_cast<Base::VectorPy*>(pnt)->value();
            getBRepOffsetAPI_MakeFillingPtr()->Add(gp_Pnt(vec.x, vec.y, vec.z));
            Py_Return;
        }
        catch (const Standard_Failure& e) {
            PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
            return nullptr;
        }
    }

    // 2nd
    PyObject* support;
    int order;
    static const std::array<const char *, 3> keywords_sup_ord {"Support", "Order", nullptr};
    PyErr_Clear();
    if (Base::Wrapped_ParseTupleAndKeywords(args, kwds, "O!i", keywords_sup_ord,
                                            &TopoShapeFacePy::Type, &support, &order)) {
        try {
            TopoDS_Face face = TopoDS::Face(static_cast<TopoShapeFacePy*>(support)->getTopoShapePtr()->getShape());
            if (face.IsNull()) {
                PyErr_SetString(PyExc_ReferenceError, "No valid face");
                return nullptr;
            }

            if (order < 0 || order > 2) {
                PyErr_SetString(PyExc_ReferenceError, "Order must be in the [0, 2] with 0 -> C0, 1 -> G1, 2 -> G2");
                return nullptr;
            }

            getBRepOffsetAPI_MakeFillingPtr()->Add(face, static_cast<GeomAbs_Shape>(order));
            Py_Return;
        }
        catch (const Standard_Failure& e) {
            PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
            return nullptr;
        }
    }

    // 3rd
    PyObject* constr;
    PyObject* isbound = Py_True;
    static const std::array<const char *, 4> keywords_const {"Constraint", "Order", "IsBound", nullptr};
    PyErr_Clear();
    if (Base::Wrapped_ParseTupleAndKeywords(args, kwds, "O!i|O!", keywords_const,
                                            &TopoShapeEdgePy::Type, &constr,
                                            &order, &PyBool_Type, isbound)) {
        try {
            TopoDS_Edge edge = TopoDS::Edge(static_cast<TopoShapeEdgePy*>(constr)->getTopoShapePtr()->getShape());
            if (edge.IsNull()) {
                PyErr_SetString(PyExc_ReferenceError, "No valid constraint edge");
                return nullptr;
            }

            if (order < 0 || order > 2) {
                PyErr_SetString(PyExc_ReferenceError, "Order must be in the [0, 2] with 0 -> C0, 1 -> G1, 2 -> G2");
                return nullptr;
            }

            getBRepOffsetAPI_MakeFillingPtr()->Add(edge, static_cast<GeomAbs_Shape>(order),
                                                   Base::asBoolean(isbound));
            Py_Return;
        }
        catch (const Standard_Failure& e) {
            PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
            return nullptr;
        }
    }

    // 4th
    static const std::array<const char *, 5> keywords_const_sup {"Constraint", "Support", "Order", "IsBound", nullptr};
    PyErr_Clear();
    if (Base::Wrapped_ParseTupleAndKeywords(args, kwds, "O!O!i|O!", keywords_const_sup,
                                            &TopoShapeEdgePy::Type, &constr,
                                            &TopoShapeFacePy::Type, &support,
                                            &order, &PyBool_Type, isbound)) {
        try {
            TopoDS_Edge edge = TopoDS::Edge(static_cast<TopoShapeEdgePy*>(constr)->getTopoShapePtr()->getShape());
            if (edge.IsNull()) {
                PyErr_SetString(PyExc_ReferenceError, "No valid constraint edge");
                return nullptr;
            }
            TopoDS_Face face = TopoDS::Face(static_cast<TopoShapeFacePy*>(support)->getTopoShapePtr()->getShape());
            if (face.IsNull()) {
                PyErr_SetString(PyExc_ReferenceError, "No valid face");
                return nullptr;
            }

            if (order < 0 || order > 2) {
                PyErr_SetString(PyExc_ReferenceError, "Order must be in the [0, 2] with 0 -> C0, 1 -> G1, 2 -> G2");
                return nullptr;
            }

            getBRepOffsetAPI_MakeFillingPtr()->Add(edge, face, static_cast<GeomAbs_Shape>(order),
                                                   Base::asBoolean(isbound));
            Py_Return;
        }
        catch (const Standard_Failure& e) {
            PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
            return nullptr;
        }
    }

    // 5th
    double u, v;
    static const std::array<const char *, 5> keywords_uv {"U", "V", "Support", "Order", nullptr};
    PyErr_Clear();
    if (Base::Wrapped_ParseTupleAndKeywords(args, kwds, "ddO!i", keywords_uv,
                                            &u, &v, &TopoShapeFacePy::Type, &support, &order)) {
        try {
            TopoDS_Face face = TopoDS::Face(static_cast<TopoShapeFacePy*>(support)->getTopoShapePtr()->getShape());
            if (face.IsNull()) {
                PyErr_SetString(PyExc_ReferenceError, "No valid face");
                return nullptr;
            }

            if (order < 0 || order > 2) {
                PyErr_SetString(PyExc_ReferenceError, "Order must be in the [0, 2] with 0 -> C0, 1 -> G1, 2 -> G2");
                return nullptr;
            }

            getBRepOffsetAPI_MakeFillingPtr()->Add(u, v, face, static_cast<GeomAbs_Shape>(order));
            Py_Return;
        }
        catch (const Standard_Failure& e) {
            PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
            return nullptr;
        }
    }

    PyErr_SetString(PyExc_TypeError, "wrong argument");
    return nullptr;
}

PyObject* BRepOffsetAPI_MakeFillingPy::build(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    try {
        getBRepOffsetAPI_MakeFillingPtr()->Build();
        Py_Return;
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BRepOffsetAPI_MakeFillingPy::isDone(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    try {
        Standard_Boolean ok = getBRepOffsetAPI_MakeFillingPtr()->IsDone();
        return Py_BuildValue("O", (ok ? Py_True : Py_False));
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BRepOffsetAPI_MakeFillingPy::G0Error(PyObject *args)
{
    int index = 0;
    if (!PyArg_ParseTuple(args, "|i", &index))
        return nullptr;

    try {
        Standard_Real v = index < 1 ? getBRepOffsetAPI_MakeFillingPtr()->G0Error()
                                    : getBRepOffsetAPI_MakeFillingPtr()->G0Error(index);
        return PyFloat_FromDouble(v);
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BRepOffsetAPI_MakeFillingPy::G1Error(PyObject *args)
{
    int index = 0;
    if (!PyArg_ParseTuple(args, "|i", &index))
        return nullptr;

    try {
        Standard_Real v = index < 1 ? getBRepOffsetAPI_MakeFillingPtr()->G1Error()
                                    : getBRepOffsetAPI_MakeFillingPtr()->G1Error(index);
        return PyFloat_FromDouble(v);
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BRepOffsetAPI_MakeFillingPy::G2Error(PyObject *args)
{
    int index = 0;
    if (!PyArg_ParseTuple(args, "|i", &index))
        return nullptr;

    try {
        Standard_Real v = index < 1 ? getBRepOffsetAPI_MakeFillingPtr()->G2Error()
                                    : getBRepOffsetAPI_MakeFillingPtr()->G2Error(index);
        return PyFloat_FromDouble(v);
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BRepOffsetAPI_MakeFillingPy::shape(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    try {
        const TopoDS_Shape& shape = this->getBRepOffsetAPI_MakeFillingPtr()->Shape();
        return new TopoShapePy(new TopoShape(shape));
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return nullptr;
    }
}

PyObject *BRepOffsetAPI_MakeFillingPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int BRepOffsetAPI_MakeFillingPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
