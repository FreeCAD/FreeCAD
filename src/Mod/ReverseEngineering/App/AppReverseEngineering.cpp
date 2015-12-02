/***************************************************************************
 *   Copyright (c) 2008 Jürgen Riegel (juergen.riegel@web.de)              *
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
# include <Python.h>
# include <TColgp_Array1OfPnt.hxx>
# include <Handle_Geom_BSplineSurface.hxx>
#endif

#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <Base/PyObjectBase.h>
#include <Base/GeometryPyCXX.h>

#include <CXX/Extensions.hxx>
#include <CXX/Objects.hxx>

#include <Mod/Part/App/BSplineSurfacePy.h>
#include <Mod/Mesh/App/Mesh.h>
#include <Mod/Mesh/App/MeshPy.h>
#include <Mod/Points/App/PointsPy.h>

#include "ApproxSurface.h"
#include "BSplineFitting.h"
#include "SurfaceTriangulation.h"

using namespace Reen;

namespace Reen {
class Module : public Py::ExtensionModule<Module>
{
public:
    Module() : Py::ExtensionModule<Module>("ReverseEngineering")
    {
        add_keyword_method("approxSurface",&Module::approxSurface,
            "approxSurface(Points=,UDegree=3,VDegree=3,NbUPoles=6,NbVPoles=6,Smooth=True)\n"
            "Weight=0.1,Grad=1.0,Bend=0.0,\n"
            "Iterations=5,Correction=True,PatchFactor=1.0"
        );
#if defined(HAVE_PCL_SURFACE)
        add_varargs_method("triangulate",&Module::triangulate,
            "triangulate(PointKernel,searchRadius[,mu=2.5])."
        );
        add_keyword_method("poissonReconstruction",&Module::poissonReconstruction,
            "poissonReconstruction(PointKernel)."
        );
#endif
#if defined(HAVE_PCL_OPENNURBS)
        add_keyword_method("fitBSpline",&Module::fitBSpline,
            "fitBSpline(PointKernel)."
        );
#endif
        initialize("This module is the ReverseEngineering module."); // register with Python
    }

    virtual ~Module() {}

private:
    Py::Object approxSurface(const Py::Tuple& args, const Py::Dict& kwds)
    {
        PyObject *o;
        PyObject *uvdirs = 0;
        // spline parameters
        int uDegree = 3;
        int vDegree = 3;
        int uPoles = 6;
        int vPoles = 6;
        // smoothing
        PyObject* smooth = Py_True;
        double weight = 0.1;
        double grad = 1.0;  //0.5
        double bend = 0.0; //0.2
        double curv = 0.0; //0.3
        // other parameters
        int iteration = 5;
        PyObject* correction = Py_True;
        double factor = 1.0;

        static char* kwds_approx[] = {"Points", "UDegree", "VDegree", "NbUPoles", "NbVPoles",
                                      "Smooth", "Weight", "Grad", "Bend", "Curv",
                                      "Iterations", "Correction", "PatchFactor","UVDirs", NULL};
        if (!PyArg_ParseTupleAndKeywords(args.ptr(), kwds.ptr(), "O|iiiiO!ddddiO!dO!",kwds_approx,
                                        &o,&uDegree,&vDegree,&uPoles,&vPoles,
                                        &PyBool_Type,&smooth,&weight,&grad,&bend,&curv,
                                        &iteration,&PyBool_Type,&correction,&factor,
                                        &PyTuple_Type,&uvdirs))
            throw Py::Exception();

        int uOrder = uDegree + 1;
        int vOrder = vDegree + 1;

        // error checking
        if (grad < 0.0 || grad > 1.0) {
            throw Py::ValueError("Value of Grad out of range [0,1]");
        }
        if (bend < 0.0 || bend > 1.0) {
            throw Py::ValueError("Value of Bend out of range [0,1]");
        }
        if (curv < 0.0 || curv > 1.0) {
            throw Py::ValueError("Value of Curv out of range [0,1]");
        }
        if (uDegree < 1 || uOrder > uPoles) {
            throw Py::ValueError("Value of uDegree out of range [1,NbUPoles-1]");
        }
        if (vDegree < 1 || vOrder > vPoles) {
            throw Py::ValueError("Value of vDegree out of range [1,NbVPoles-1]");
        }

        double sum = (grad + bend + curv);
        if (sum > 0)
            weight = weight / sum;

        try {
            std::vector<Base::Vector3f> pts;
            if (PyObject_TypeCheck(o, &(Points::PointsPy::Type))) {
                Points::PointsPy* pPoints = static_cast<Points::PointsPy*>(o);
                Points::PointKernel* points = pPoints->getPointKernelPtr();
                pts = points->getBasicPoints();
            }
            else {
                Py::Sequence l(o);
                pts.reserve(l.size());
                for (Py::Sequence::iterator it = l.begin(); it != l.end(); ++it) {
                    Py::Tuple t(*it);
                    pts.push_back(Base::Vector3f(
                        (float)Py::Float(t.getItem(0)),
                        (float)Py::Float(t.getItem(1)),
                        (float)Py::Float(t.getItem(2)))
                    );
                }
            }

            TColgp_Array1OfPnt clPoints(0, pts.size()-1);
            if (clPoints.Length() < uPoles * vPoles) {
                throw Py::ValueError("Too less data points for the specified number of poles");
            }

            int index=0;
            for (std::vector<Base::Vector3f>::iterator it = pts.begin(); it != pts.end(); ++it) {
                clPoints(index++) = gp_Pnt(it->x, it->y, it->z);
            }

            Reen::BSplineParameterCorrection pc(uOrder,vOrder,uPoles,vPoles);
            Handle_Geom_BSplineSurface hSurf;

            if (uvdirs) {
                Py::Tuple t(uvdirs);
                Base::Vector3d u = Py::Vector(t.getItem(0)).toVector();
                Base::Vector3d v = Py::Vector(t.getItem(1)).toVector();
                pc.SetUV(u, v);
            }
            pc.EnableSmoothing(PyObject_IsTrue(smooth) ? true : false, weight, grad, bend, curv);
            hSurf = pc.CreateSurface(clPoints, iteration, PyObject_IsTrue(correction) ? true : false, factor);
            if (!hSurf.IsNull()) {
                return Py::asObject(new Part::BSplineSurfacePy(new Part::GeomBSplineSurface(hSurf)));
            }

            throw Py::RuntimeError("Computation of B-Spline surface failed");
        }
        catch (const Py::Exception&) {
            // re-throw
            throw;
        }
        catch (Standard_Failure &e) {
            std::string str;
            Standard_CString msg = e.GetMessageString();
            str += typeid(e).name();
            str += " ";
            if (msg) {str += msg;}
            else     {str += "No OCCT Exception Message";}
            throw Py::RuntimeError(str);
        }
        catch (const Base::Exception &e) {
            throw Py::RuntimeError(e.what());
        }
        catch (...) {
            throw Py::RuntimeError("Unknown C++ exception");
        }
    }
#if defined(HAVE_PCL_SURFACE)
    Py::Object triangulate(const Py::Tuple& args)
    {
        PyObject *pcObj;
        double searchRadius;
        double mu=2.5;
        if (!PyArg_ParseTuple(args.ptr(), "O!d|d", &(Points::PointsPy::Type), &pcObj, &searchRadius, &mu))
            throw Py::Exception();

        Points::PointsPy* pPoints = static_cast<Points::PointsPy*>(pcObj);
        Points::PointKernel* points = pPoints->getPointKernelPtr();
        
        Mesh::MeshObject* mesh = new Mesh::MeshObject();
        SurfaceTriangulation tria(*points, *mesh);
        tria.perform(searchRadius, mu);

        return Py::asObject(new Mesh::MeshPy(mesh));
    }
    Py::Object poissonReconstruction(const Py::Tuple& args, const Py::Dict& kwds)
    {
        PyObject *pcObj;
        int ksearch=5;
        int octreeDepth=-1;
        int solverDivide=-1;
        double samplesPerNode=-1.0;

        static char* kwds_poisson[] = {"Points", "KSearch", "OctreeDepth", "SolverDivide",
                                      "SamplesPerNode", NULL};
        if (!PyArg_ParseTupleAndKeywords(args.ptr(), kwds.ptr(), "O!|iiid", kwds_poisson,
                                        &(Points::PointsPy::Type), &pcObj,
                                        &ksearch, &octreeDepth, &solverDivide, &samplesPerNode))
            throw Py::Exception();

        Points::PointsPy* pPoints = static_cast<Points::PointsPy*>(pcObj);
        Points::PointKernel* points = pPoints->getPointKernelPtr();

        Mesh::MeshObject* mesh = new Mesh::MeshObject();
        Reen::PoissonReconstruction poisson(*points, *mesh);
        poisson.setDepth(octreeDepth);
        poisson.setSolverDivide(solverDivide);
        poisson.setSamplesPerNode(samplesPerNode);
        poisson.perform(ksearch);

        return Py::asObject(new Mesh::MeshPy(mesh));
    }
#endif
#if defined(HAVE_PCL_OPENNURBS)
    Py::Object fitBSpline(const Py::Tuple& args, const Py::Dict& kwds)
    {
        PyObject *pcObj;
        int degree = 2;
        int refinement = 4;
        int iterations = 10;
        double interiorSmoothness = 0.2;
        double interiorWeight = 1.0;
        double boundarySmoothness = 0.2;
        double boundaryWeight = 0.0;

        static char* kwds_approx[] = {"Points", "Degree", "Refinement", "Iterations",
                                      "InteriorSmoothness", "InteriorWeight", "BoundarySmoothness", "BoundaryWeight", NULL};
        if (!PyArg_ParseTupleAndKeywords(args.ptr(), kwds.ptr(), "O!|iiidddd", kwds_approx,
                                        &(Points::PointsPy::Type), &pcObj,
                                        &degree, &refinement, &iterations,
                                        &interiorSmoothness, &interiorWeight,
                                        &boundarySmoothness, &boundaryWeight))
            throw Py::Exception();

        Points::PointsPy* pPoints = static_cast<Points::PointsPy*>(pcObj);
        Points::PointKernel* points = pPoints->getPointKernelPtr();

        BSplineFitting fit(points->getBasicPoints());
        fit.setOrder(degree+1);
        fit.setRefinement(refinement);
        fit.setIterations(iterations);
        fit.setInteriorSmoothness(interiorSmoothness);
        fit.setInteriorWeight(interiorWeight);
        fit.setBoundarySmoothness(boundarySmoothness);
        fit.setBoundaryWeight(boundaryWeight);
        Handle(Geom_BSplineSurface) hSurf = fit.perform();

        if (!hSurf.IsNull()) {
            return Py::asObject(new Part::BSplineSurfacePy(new Part::GeomBSplineSurface(hSurf)));
        }

        throw Py::RuntimeError("Computation of B-Spline surface failed");
    }
#endif
};
} // namespace Reen


/* Python entry */
extern "C" {
void ReenExport initReverseEngineering()
{
    // load dependent module
    try {
        Base::Interpreter().loadModule("Part");
        Base::Interpreter().loadModule("Mesh");
    }
    catch(const Base::Exception& e) {
        PyErr_SetString(PyExc_ImportError, e.what());
        return;
    }

    new Reen::Module();
    Base::Console().Log("Loading ReverseEngineering module... done\n");
}

} // extern "C"
