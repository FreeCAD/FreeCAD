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

#include <CXX/Extensions.hxx>
#include <CXX/Objects.hxx>

#include <Mod/Part/App/BSplineSurfacePy.h>
#include <Mod/Mesh/App/Mesh.h>
#include <Mod/Mesh/App/MeshPy.h>
#include <Mod/Points/App/PointsPy.h>

#include "ApproxSurface.h"
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
#endif
        initialize("This module is the ReverseEngineering module."); // register with Python
    }

    virtual ~Module() {}

private:
    Py::Object approxSurface(const Py::Tuple& args, const Py::Dict& kwds)
    {
        PyObject *o;
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
        // other parameters
        int iteration = 5;
        PyObject* correction = Py_True;
        double factor = 1.0;

        static char* kwds_approx[] = {"Points", "UDegree", "VDegree", "NbUPoles", "NbVPoles",
                                      "Smooth", "Weight", "Grad", "Bend",
                                      "Iterations", "Correction", "PatchFactor", NULL};
        if (!PyArg_ParseTupleAndKeywords(args.ptr(), kwds.ptr(), "O|iiiiO!dddiO!d",kwds_approx,
                                        &o,&uDegree,&vDegree,&uPoles,&vPoles,
                                        &PyBool_Type,&smooth,&weight,&grad,&bend,
                                        &iteration,&PyBool_Type,&correction,&factor))
            throw Py::Exception();

        double curvdiv = 1.0 - (grad + bend);
        int uOrder = uDegree + 1;
        int vOrder = vDegree + 1;

        // error checking
        if (grad < 0.0 || grad > 1.0) {
            throw Py::ValueError("Value of Grad out of range [0,1]");
        }
        if (bend < 0.0 || bend > 1.0) {
            throw Py::ValueError("Value of Bend out of range [0,1]");
        }
        if (curvdiv < 0.0 || curvdiv > 1.0) {
            throw Py::ValueError("Sum of Grad and Bend out of range [0,1]");
        }
        if (uDegree < 1 || uOrder > uPoles) {
            throw Py::ValueError("Value of uDegree out of range [1,NbUPoles-1]");
        }
        if (vDegree < 1 || vOrder > vPoles) {
            throw Py::ValueError("Value of vDegree out of range [1,NbVPoles-1]");
        }

        try {
            Py::Sequence l(o);
            TColgp_Array1OfPnt clPoints(0, l.size()-1);

            int index=0;
            for (Py::Sequence::iterator it = l.begin(); it != l.end(); ++it) {
                Py::Tuple t(*it);
                clPoints(index++) = gp_Pnt(
                    (double)Py::Float(t.getItem(0)),
                    (double)Py::Float(t.getItem(1)),
                    (double)Py::Float(t.getItem(2)));
            }

            Reen::BSplineParameterCorrection pc(uOrder,vOrder,uPoles,vPoles);
            Handle_Geom_BSplineSurface hSurf;

            pc.EnableSmoothing(PyObject_IsTrue(smooth) ? true : false, weight, grad, bend, curvdiv);
            hSurf = pc.CreateSurface(clPoints, iteration, PyObject_IsTrue(correction) ? true : false, factor);
            if (!hSurf.IsNull()) {
                return Py::asObject(new Part::BSplineSurfacePy(new Part::GeomBSplineSurface(hSurf)));
            }

            throw Py::RuntimeError("Computation of B-Spline surface failed");
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
