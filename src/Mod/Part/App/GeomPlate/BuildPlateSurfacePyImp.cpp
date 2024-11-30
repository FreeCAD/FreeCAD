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
# include <Standard_Failure.hxx>
#endif

#include "GeomPlate/BuildPlateSurfacePy.h"
#include "GeomPlate/BuildPlateSurfacePy.cpp"
#include "GeomPlate/CurveConstraintPy.h"
#include "GeomPlate/PointConstraintPy.h"
#include "Geometry2d.h"
#include "GeometrySurfacePy.h"

#include <Base/PyWrapParseTupleAndKeywords.h>


using namespace Part;

/*!
 * \brief BuildPlateSurfacePy::PyMake
 * \code
v1=App.Vector(0,0,0)
v2=App.Vector(10,0,0)
v3=App.Vector(10,10,3)
v4=App.Vector(0,10,0)
v5=App.Vector(5,5,5)

l1=Part.LineSegment(v1, v2)
l2=Part.LineSegment(v2, v3)
l3=Part.LineSegment(v3, v4)
l4=Part.LineSegment(v4, v1)

c1=Part.GeomPlate.CurveConstraint(l1)
c2=Part.GeomPlate.CurveConstraint(l2)
c3=Part.GeomPlate.CurveConstraint(l3)
c4=Part.GeomPlate.CurveConstraint(l4)
c5=Part.GeomPlate.PointConstraint(v5)

bp=Part.GeomPlate.BuildPlateSurface()
bp.add(c1)
bp.add(c2)
bp.add(c3)
bp.add(c4)
bp.add(c5)
bp.perform()
s=bp.surface()
bs=s.makeApprox()
Part.show(bs.toShape())
Part.show(l1.toShape())
Part.show(l2.toShape())
Part.show(l3.toShape())
Part.show(l4.toShape())

bp.surfInit()
 * \endcode
 */
PyObject *BuildPlateSurfacePy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of BuildPlateSurfacePy
    return new BuildPlateSurfacePy(nullptr);
}

// constructor method
int BuildPlateSurfacePy::PyInit(PyObject* args, PyObject* kwds)
{
    PyObject *surf = nullptr;
    int degree = 3;
    int nbPtsOnCur = 10;
    int nbIter = 3;
    double tol2d = 0.00001;
    double tol3d = 0.0001;
    double tolAng = 0.01;
    double tolCurv = 0.1;
    PyObject* anisotropy = Py_False;

    static const std::array<const char *, 10> keywords{"Surface", "Degree", "NbPtsOnCur", "NbIter", "Tol2d", "Tol3d",
                                                       "TolAng", "TolCurv", "Anisotropy", nullptr};
    if (!Base::Wrapped_ParseTupleAndKeywords(args, kwds, "|O!iiiddddO!", keywords,
                                             &(GeometrySurfacePy::Type), &surf, &degree,
                                             &nbPtsOnCur, &nbIter, &tol2d, &tol3d,
                                             &tolAng, &tolCurv, &PyBool_Type, &anisotropy)) {
        return -1;
    }

    try {
        std::unique_ptr<GeomPlate_BuildPlateSurface> ptr(new GeomPlate_BuildPlateSurface
                                                         (degree, nbPtsOnCur, nbIter, tol2d, tol3d, tolAng, tolCurv,
                                                          Base::asBoolean(anisotropy)));

        if (surf) {
            GeomSurface* surface = static_cast<GeometrySurfacePy*>(surf)->getGeomSurfacePtr();
            Handle(Geom_Surface) handle = Handle(Geom_Surface)::DownCast(surface->handle());
            if (handle.IsNull()) {
                PyErr_SetString(PyExc_ReferenceError, "No valid surface handle");
                return -1;
            }
            ptr->LoadInitSurface(handle);
        }

        setTwinPointer(ptr.release());

        return 0;
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return -1;
    }
}

// returns a string which represents the object e.g. when printed in python
std::string BuildPlateSurfacePy::representation() const
{
    return {"<GeomPlate_BuildPlateSurface object>"};
}

PyObject* BuildPlateSurfacePy::init(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    try {
        getGeomPlate_BuildPlateSurfacePtr()->Init();
        Py_Return;
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BuildPlateSurfacePy::loadInitSurface(PyObject *args)
{
    PyObject* surf;
    if (!PyArg_ParseTuple(args, "O!", &(GeometrySurfacePy::Type), &surf))
        return nullptr;

    GeomSurface* surface = static_cast<GeometrySurfacePy*>(surf)->getGeomSurfacePtr();
    Handle(Geom_Surface) handle = Handle(Geom_Surface)::DownCast(surface->handle());
    if (handle.IsNull()) {
        PyErr_SetString(PyExc_ReferenceError, "No valid surface handle");
        return nullptr;
    }

    try {
        getGeomPlate_BuildPlateSurfacePtr()->LoadInitSurface(handle);
        Py_Return;
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BuildPlateSurfacePy::add(PyObject *args)
{
    PyObject* cont;
    if (!PyArg_ParseTuple(args, "O", &cont))
        return nullptr;

    try {
        if (PyObject_TypeCheck(cont, &PointConstraintPy::Type)) {
            GeomPlate_PointConstraint* pc = static_cast<PointConstraintPy*>(cont)->getGeomPlate_PointConstraintPtr();
            getGeomPlate_BuildPlateSurfacePtr()->Add(new GeomPlate_PointConstraint(*pc));
            Py_Return;
        }
        else if (PyObject_TypeCheck(cont, &CurveConstraintPy::Type)) {
            GeomPlate_CurveConstraint* cc = static_cast<CurveConstraintPy*>(cont)->getGeomPlate_CurveConstraintPtr();
            getGeomPlate_BuildPlateSurfacePtr()->Add(new GeomPlate_CurveConstraint(*cc));
            Py_Return;
        }
        else {
            PyErr_SetString(PyExc_TypeError, "PointConstraint or CurveConstraint expected");
            return nullptr;
        }
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BuildPlateSurfacePy::setNbBounds(PyObject *args)
{
    int count;
    if (!PyArg_ParseTuple(args, "i", &count))
        return nullptr;

    try {
        getGeomPlate_BuildPlateSurfacePtr()->SetNbBounds(count);
        Py_Return;
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BuildPlateSurfacePy::perform(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    try {
        getGeomPlate_BuildPlateSurfacePtr()->Perform();
        Py_Return;
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BuildPlateSurfacePy::isDone(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    try {
        Standard_Boolean ok = getGeomPlate_BuildPlateSurfacePtr()->IsDone();
        return Py_BuildValue("O", (ok ? Py_True : Py_False));
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BuildPlateSurfacePy::surface(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    try {
        Handle(Geom_Surface) hSurf = getGeomPlate_BuildPlateSurfacePtr()->Surface();
        if (hSurf.IsNull())
            Py_Return;

        std::unique_ptr<GeomSurface> geo(makeFromSurface(hSurf));
        return geo->getPyObject();
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BuildPlateSurfacePy::surfInit(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    try {
        Handle(Geom_Surface) hSurf = getGeomPlate_BuildPlateSurfacePtr()->SurfInit();
        if (hSurf.IsNull())
            Py_Return;

        std::unique_ptr<GeomSurface> geo(makeFromSurface(hSurf));
        return geo->getPyObject();
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BuildPlateSurfacePy::curveConstraint(PyObject *args)
{
    int index;
    if (!PyArg_ParseTuple(args, "i", &index))
        return nullptr;

    try {
        Handle(GeomPlate_CurveConstraint) hCC = getGeomPlate_BuildPlateSurfacePtr()->CurveConstraint(index);
        if (hCC.IsNull())
            Py_Return;

        std::unique_ptr<GeomPlate_CurveConstraint> ptr(new GeomPlate_CurveConstraint(*hCC));
        return new CurveConstraintPy(ptr.release());
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BuildPlateSurfacePy::pointConstraint(PyObject *args)
{
    int index;
    if (!PyArg_ParseTuple(args, "i", &index))
        return nullptr;

    try {
        Handle(GeomPlate_PointConstraint) hPC = getGeomPlate_BuildPlateSurfacePtr()->PointConstraint(index);
        if (hPC.IsNull())
            Py_Return;

        std::unique_ptr<GeomPlate_PointConstraint> ptr(new GeomPlate_PointConstraint(*hPC));
        return new PointConstraintPy(ptr.release());
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BuildPlateSurfacePy::disc2dContour(PyObject *args)
{
    int index;
    if (!PyArg_ParseTuple(args, "i", &index))
        return nullptr;

    try {
        TColgp_SequenceOfXY seq2d;
        getGeomPlate_BuildPlateSurfacePtr()->Disc2dContour(index, seq2d);

        Py::List list;
        for (int i = seq2d.Lower(); i <= seq2d.Upper(); ++i) {
            const gp_XY& pnt = seq2d.Value(i);
            Py::Tuple coord(2);
            coord.setItem(0, Py::Float(pnt.X()));
            coord.setItem(1, Py::Float(pnt.Y()));
            list.append(coord);
        }

        return Py::new_reference_to(list);
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BuildPlateSurfacePy::disc3dContour(PyObject *args)
{
    int index, order;
    if (!PyArg_ParseTuple(args, "ii", &index, &order))
        return nullptr;

    try {
        TColgp_SequenceOfXYZ seq3d;
        getGeomPlate_BuildPlateSurfacePtr()->Disc3dContour(index, order, seq3d);

        Py::List list;
        for (int i = seq3d.Lower(); i <= seq3d.Upper(); ++i) {
            const gp_XYZ& pnt = seq3d.Value(i);
            Py::Tuple coord(3);
            coord.setItem(0, Py::Float(pnt.X()));
            coord.setItem(1, Py::Float(pnt.Y()));
            coord.setItem(2, Py::Float(pnt.Z()));
            list.append(coord);
        }

        return Py::new_reference_to(list);
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BuildPlateSurfacePy::sense(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    try {
        Handle(TColStd_HArray1OfInteger) hOrder = getGeomPlate_BuildPlateSurfacePtr()->Sense();
        Py::List list;
        if (!hOrder.IsNull()) {
            for (auto i = hOrder->Lower(); i <= hOrder->Upper(); ++i) {
                list.append(Py::Long(hOrder->Value(i)));
            }
        }
        return Py::new_reference_to(list);
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BuildPlateSurfacePy::curves2d(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    try {
        Handle(TColGeom2d_HArray1OfCurve) hCurves = getGeomPlate_BuildPlateSurfacePtr()->Curves2d();
        Py::List list;
        if (!hCurves.IsNull()) {
            for (auto i = hCurves->Lower(); i <= hCurves->Upper(); ++i) {
                Handle(Geom2d_Curve) hCurve = hCurves->Value(i);
                std::unique_ptr<Geom2dCurve> ptr(makeFromCurve2d(hCurve));
                if (ptr)
                    list.append(Py::asObject(ptr->getPyObject()));
            }
        }
        return Py::new_reference_to(list);
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BuildPlateSurfacePy::order(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    try {
        Handle(TColStd_HArray1OfInteger) hOrder = getGeomPlate_BuildPlateSurfacePtr()->Order();
        Py::List list;
        if (!hOrder.IsNull()) {
            for (auto i = hOrder->Lower(); i <= hOrder->Upper(); ++i) {
                list.append(Py::Long(hOrder->Value(i)));
            }
        }
        return Py::new_reference_to(list);
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BuildPlateSurfacePy::G0Error(PyObject *args)
{
    int index = 0;
    if (!PyArg_ParseTuple(args, "|i", &index))
        return nullptr;

    try {
        Standard_Real v = index < 1 ? getGeomPlate_BuildPlateSurfacePtr()->G0Error()
                                    : getGeomPlate_BuildPlateSurfacePtr()->G0Error(index);
        return PyFloat_FromDouble(v);
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BuildPlateSurfacePy::G1Error(PyObject *args)
{
    int index = 0;
    if (!PyArg_ParseTuple(args, "|i", &index))
        return nullptr;

    try {
        Standard_Real v = index < 1 ? getGeomPlate_BuildPlateSurfacePtr()->G1Error()
                                    : getGeomPlate_BuildPlateSurfacePtr()->G1Error(index);
        return PyFloat_FromDouble(v);
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return nullptr;
    }
}

PyObject* BuildPlateSurfacePy::G2Error(PyObject *args)
{
    int index = 0;
    if (!PyArg_ParseTuple(args, "|i", &index))
        return nullptr;

    try {
        Standard_Real v = index < 1 ? getGeomPlate_BuildPlateSurfacePtr()->G2Error()
                                    : getGeomPlate_BuildPlateSurfacePtr()->G2Error(index);
        return PyFloat_FromDouble(v);
    }
    catch (const Standard_Failure& e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
        return nullptr;
    }
}

PyObject *BuildPlateSurfacePy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int BuildPlateSurfacePy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
