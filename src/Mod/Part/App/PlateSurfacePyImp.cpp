/***************************************************************************
 *   Copyright (c) 2015 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <GeomPlate_BuildPlateSurface.hxx>
# include <GeomPlate_MakeApprox.hxx>
# include <GeomPlate_PointConstraint.hxx>
# include <GeomPlate_Surface.hxx>
#endif

#include <Base/GeometryPyCXX.h>
#include <Base/PyWrapParseTupleAndKeywords.h>
#include <Base/VectorPy.h>
#include <Base/Vector3D.h>

#include "PlateSurfacePy.h"
#include "PlateSurfacePy.cpp"
#include "BSplineSurfacePy.h"
#include "OCCError.h"


using namespace Part;

// returns a string which represents the object e.g. when printed in python
std::string PlateSurfacePy::representation() const
{
    return "<PlateSurface object>";
}

PyObject *PlateSurfacePy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of PlateSurfacePy and the Twin object
    return new PlateSurfacePy(new GeomPlateSurface);
}

// constructor method
int PlateSurfacePy::PyInit(PyObject* args, PyObject* kwds)
{
    static const std::array<const char *, 12> kwds_Parameter{"Surface", "Points", "Curves", "Degree",
                                                             "NbPtsOnCur", "NbIter", "Tol2d", "Tol3d", "TolAng",
                                                             "TolCurv", "Anisotropie", nullptr};

    PyObject* surface = nullptr;
    PyObject* points = nullptr;
    PyObject* curves = nullptr;
    int Degree = 3;
    int NbPtsOnCur = 10;
    int NbIter = 3;
    double Tol2d = 0.00001;
    double Tol3d = 0.0001;
    double TolAng = 0.01;
    double TolCurv = 0.1;
    PyObject* Anisotropie = Py_False; // NOLINT

    if (!Base::Wrapped_ParseTupleAndKeywords(args, kwds, "|O!OOiiiddddO!", kwds_Parameter,
                                             &(GeometryPy::Type), &surface, &points, &curves,
                                             &Degree, &NbPtsOnCur, &NbIter, &Tol2d, &Tol3d, &TolAng, &TolCurv,
                                             &PyBool_Type, &Anisotropie)) {
        return -1;
    }

    if (!surface && !points && !curves) {
        PyErr_SetString(PyExc_ValueError, "set points or curves as constraints");
        return -1;
    }

    Handle(Geom_Surface) surf;
    if (surface) {
        GeometryPy* pcGeo = static_cast<GeometryPy*>(surface);
        surf = Handle(Geom_Surface)::DownCast
            (pcGeo->getGeometryPtr()->handle());
        if (surf.IsNull()) {
            PyErr_SetString(PyExc_TypeError, "geometry is not a surface");
            return -1;
        }
    }

    try {
        GeomPlate_BuildPlateSurface buildPlate(Degree, NbPtsOnCur, NbIter, Tol2d, Tol3d, TolAng, TolCurv,
            Base::asBoolean(Anisotropie));
        if (!surf.IsNull()) {
            buildPlate.LoadInitSurface(surf);

            if (!points && !curves) {
                Standard_Real U1,U2,V1,V2;
                surf->Bounds(U1,U2,V1,V2);
                buildPlate.Add(new GeomPlate_PointConstraint(surf->Value(U1,V1),0));
                buildPlate.Add(new GeomPlate_PointConstraint(surf->Value(U1,V2),0));
                buildPlate.Add(new GeomPlate_PointConstraint(surf->Value(U2,V1),0));
                buildPlate.Add(new GeomPlate_PointConstraint(surf->Value(U2,V2),0));
            }
        }

        if (points) {
            Py::Sequence list(points);
            for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
                Base::Vector3d vec = Py::Vector(*it).toVector();
                Handle(GeomPlate_PointConstraint) PCont = new GeomPlate_PointConstraint(gp_Pnt(vec.x,vec.y,vec.z),0);
                buildPlate.Add(PCont);
            }
        }

        if (curves) {
            Py::Sequence list(curves);
            for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
                //TODO
            }
        }

        buildPlate.Perform();
        getGeomPlateSurfacePtr()->setHandle(buildPlate.Surface());
        return 0;
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return -1;
    }
}

PyObject* PlateSurfacePy::makeApprox(PyObject *args, PyObject* kwds)
{
    static const std::array<const char *, 8> kwds_Parameter{"Tol3d", "MaxSegments", "MaxDegree", "MaxDistance",
                                                            "CritOrder", "Continuity", "EnlargeCoeff", nullptr};

    double tol3d=0.01;
    int maxSeg=9;
    int maxDegree=3;
    double dmax = 0.0001;
    int critOrder=0;
    char* cont = "C1";
    double enlargeCoeff = 1.1;

    if (!Base::Wrapped_ParseTupleAndKeywords(args, kwds, "|diidisd", kwds_Parameter,
                                             &tol3d, &maxSeg, &maxDegree, &dmax, &critOrder, &cont, &enlargeCoeff)) {
        return nullptr;
    }

    GeomAbs_Shape continuity;
    std::string uc = cont;
    if (uc == "C0")
        continuity = GeomAbs_C0;
    else if (uc == "C1")
        continuity = GeomAbs_C1;
    else if (uc == "C2")
        continuity = GeomAbs_C2;
    else if (uc == "C3")
        continuity = GeomAbs_C3;
    else if (uc == "CN")
        continuity = GeomAbs_CN;
    else if (uc == "G1")
        continuity = GeomAbs_G1;
    else
        continuity = GeomAbs_C1;

    PY_TRY {
        GeomPlate_MakeApprox approx(Handle(GeomPlate_Surface)::DownCast(getGeomPlateSurfacePtr()->handle()),
            tol3d, maxSeg, maxDegree, dmax, critOrder, continuity, enlargeCoeff);
        Handle(Geom_BSplineSurface) hSurf = approx.Surface();

        if (!hSurf.IsNull()) {
            return new Part::BSplineSurfacePy(new Part::GeomBSplineSurface(hSurf));
        }

        PyErr_SetString(PyExc_RuntimeError, "Approximation of B-spline surface failed");
        return nullptr;
    } PY_CATCH_OCC;
}

PyObject *PlateSurfacePy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int PlateSurfacePy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
