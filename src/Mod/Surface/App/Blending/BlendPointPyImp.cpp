///***************************************************************************
// *   Copyright (c) 2022 Matteo Grellier <matteogrellier@gmail.com>         *
// *                                                                         *
// *   This file is part of the FreeCAD CAx development system.              *
// *                                                                         *
// *   This library is free software; you can redistribute it and/or         *
// *   modify it under the terms of the GNU Library General Public           *
// *   License as published by the Free Software Foundation; either          *
// *   version 2 of the License, or (at your option) any later version.      *
// *                                                                         *
// *   This library  is distributed in the hope that it will be useful,      *
// *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
// *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
// *   GNU Library General Public License for more details.                  *
// *                                                                         *
// *   You should have received a copy of the GNU Library General Public     *
// *   License along with this library; see the file COPYING.LIB. If not,    *
// *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
// *   Suite 330, Boston, MA  02111-1307, USA                                *
// *                                                                         *
// ***************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
#include <BRepAdaptor_Curve.hxx>
#include <TColStd_Array1OfReal.hxx>
#include <TColgp_Array1OfPnt.hxx>
#include <TopoDS.hxx>
#endif
#include "Blending/BlendPoint.h"
#include "Blending/BlendPointPy.h"
#include <Base/GeometryPyCXX.h>
#include <Base/VectorPy.h>
#include <Mod/Part/App/TopoShapePy.h>
#include "Blending/BlendPointPy.cpp"


using namespace Surface;

std::string BlendPointPy::representation(void) const
{
    Base::Vector3d bp = getBlendPointPtr()->vectors[0];
    std::stringstream str;
    str << "G" << getBlendPointPtr()->getContinuity()
        << " BlendPoint at (" << bp.x << ", " << bp.y << ", " << bp.z << "), ";
    return str.str();
}

PyObject *BlendPointPy::PyMake(struct _typeobject *, PyObject *, PyObject *)// Python wrapper
{
    // create a new instance of BlendPointPy
    return new BlendPointPy(new BlendPoint);
}

int BlendPointPy::PyInit(PyObject *args, PyObject *)
{
    PyObject *plist;
    std::vector<Base::Vector3d> vecs;
    if (PyArg_ParseTuple(args, "O", &plist)) {
        Py::Sequence list(plist);
        if (list.size() == 0) {
            vecs.emplace_back(Base::Vector3d(0, 0, 0));
        }
        else {
            for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
                Py::Vector v(*it);
                Base::Vector3d vec = v.toVector();
                vecs.emplace_back(vec);
            }
        }
        this->getBlendPointPtr()->vectors = vecs;
        return 0;
    }

    PyErr_Clear();
    if (PyArg_ParseTuple(args, "")) {
        vecs.emplace_back(Base::Vector3d(0, 0, 0));
        this->getBlendPointPtr()->vectors = vecs;
        return 0;
    }

    double param;
    int cont;
    PyObject *pcObj;
    PyErr_Clear();
    // Create a curve with an edge, parameter and continiuity.
    if (PyArg_ParseTuple(args, "O!di", &(Part::TopoShapePy::Type), &pcObj, &param, &cont)) {
        try {
            gp_Pnt Pt;
            TopoDS_Shape shape = static_cast<Part::TopoShapePy *>(pcObj)->getTopoShapePtr()->getShape();
            const TopoDS_Edge &e = TopoDS::Edge(shape);
            BRepAdaptor_Curve adapt(e);
            if (param < adapt.FirstParameter() || param > adapt.LastParameter()) {
                PyErr_Warn(PyExc_UserWarning, "BlendPoint: edge is not a closed curve");
                Base::Console().Message("fp=%f\n", adapt.FirstParameter());
                Base::Console().Message("lp=%f\n", adapt.LastParameter());
            }

            adapt.D0(param, Pt);
            Base::Vector3d bv(Pt.X(), Pt.Y(), Pt.Z());
            vecs.emplace_back(bv);

            for (int i = 1; i <= cont; i++) {
                gp_Vec v1 = adapt.DN(param, i);
                Base::Vector3d bbv1(v1.X(), v1.Y(), v1.Z());
                vecs.emplace_back(bbv1);
            }
            this->getBlendPointPtr()->vectors = vecs;
            return 0;
        }
        catch (const std::exception &e) {
            std::cerr << e.what() << '\n';
            return -1;
        }
    }
    return -1;
}

PyObject *BlendPointPy::setSize(PyObject *args)
{
    double size = 1.0;

    if (!PyArg_ParseTuple(args, "d", &size))
        return nullptr;
    try {
        getBlendPointPtr()->setSize(size);
        Py_Return;
    }
    catch (Standard_Failure &e) {
        PyErr_SetString(PyExc_Exception, "Failed to set size");
        return nullptr;
    }
}

PyObject *BlendPointPy::getSize(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    int nb = getBlendPointPtr()->nbVectors();
    if (nb >= 2) {
        double bpTangentLength = getBlendPointPtr()->vectors[1].Length();
        return Py_BuildValue("d", bpTangentLength);
    }
    return nullptr;
}

Py::List BlendPointPy::getVectors() const
{
    try {
        BlendPoint *bp = getBlendPointPtr();
        std::vector<Base::Vector3d> p = bp->vectors;
        Py::List vecs;
        for (size_t i = 0; i < p.size(); i++) {
            Base::VectorPy *vec = new Base::VectorPy(Base::Vector3d(
                p[i].x, p[i].y, p[i].z));
            vecs.append(Py::asObject(vec));
        }
        return vecs;
    }
    catch (Standard_Failure &e) {
        PyErr_SetString(PyExc_RuntimeError,
                        e.GetMessageString());
        return Py::List();
    }
}

PyObject *BlendPointPy::setvectors(PyObject *args)
{
    BlendPoint *bp = getBlendPointPtr();
    PyObject *plist;
    if (!PyArg_ParseTuple(args, "O", &plist)) {
        PyErr_SetString(PyExc_TypeError, "List of vectors required.");
        return nullptr;
    }
    try {
        Py::Sequence list(plist);
        std::vector<Base::Vector3d> vecs;
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            Py::Vector v(*it);
            Base::Vector3d pole = v.toVector();
            vecs.emplace_back(pole);
        }
        bp->vectors = vecs;
        Py_Return;
    }
    catch (Standard_Failure &e) {
        PyErr_SetString(PyExc_RuntimeError, e.GetMessageString());
    }
    return nullptr;
}

PyObject *BlendPointPy::getCustomAttributes(const char * /*attr*/) const
{
    return nullptr;
}

int BlendPointPy::setCustomAttributes(const char * /*attr*/, PyObject * /*obj*/)
{
    return 0;
}
