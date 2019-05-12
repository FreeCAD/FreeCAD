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
# include <BRepBuilderAPI_MakePolygon.hxx>
#endif

#include <CXX/Extensions.hxx>
#include <CXX/Objects.hxx>

#include <Base/PyObjectBase.h>
#include <Base/Console.h>
#include <Base/Vector3D.h>
#include <Base/VectorPy.h>
#include <Base/GeometryPyCXX.h>
#include <Mod/Part/App/TopoShapePy.h>
#include <Mod/Part/App/TopoShapeEdgePy.h>
#include <Mod/Part/App/TopoShapeWirePy.h>
#include <Mod/Mesh/App/Core/Algorithm.h>
#include <Mod/Mesh/App/Core/MeshKernel.h>
#include <Mod/Mesh/App/Mesh.h>
#include <Mod/Mesh/App/MeshPy.h>
#include "MeshAlgos.h"
#include "Mesher.h"

namespace MeshPart {
class Module : public Py::ExtensionModule<Module>
{
public:
    Module() : Py::ExtensionModule<Module>("MeshPart")
    {
        add_varargs_method("loftOnCurve",&Module::loftOnCurve,
            "Creates a mesh loft based on a curve and an up vector\n"
            "\n"
            "loftOnCurve(curve, poly, upVector, MaxSize)\n"
            "\n"
            "Args:\n"
            "    curve (topology):\n"
            "    poly (list of (x, y z) or (x, y) tuples of floats):\n"
            "    upVector ((x, y, z) tuple):\n"
            "    MaxSize (float):\n"
        );
        add_varargs_method("projectShapeOnMesh",&Module::projectShapeOnMesh,
            "Projects a shape onto a mesh with a given maximum distance\n"
            "projectShapeOnMesh(shape, mesh, float) -> polygon"
        );
        add_varargs_method("wireFromSegment",&Module::wireFromSegment,
            "Create wire(s) from boundary of segment\n"
        );
        add_keyword_method("meshFromShape",&Module::meshFromShape,
            "Create surface mesh from shape\n"
            "\n"
            "Multiple signatures are available:\n"
            "\n"
            "    meshFromShape(Shape)\n"
            "    meshFromShape(Shape, LinearDeflection,\n"
            "                         AngularDeflection=0.5,\n"
            "                         Relative=False,"
            "                         Segments=False,\n"
            "                         GroupColors=[])\n"
            "    meshFromShape(Shape, MaxLength)\n"
            "    meshFromShape(Shape, MaxArea)\n"
            "    meshFromShape(Shape, LocalLength)\n"
            "    meshFromShape(Shape, Deflection)\n"
            "    meshFromShape(Shape, MinLength, MaxLength)\n"
            "\n"
            "Additionally, when FreeCAD is built with netgen, the following\n"
            "signatures are also available (they are "
#ifndef HAVE_NETGEN
            "NOT "
#endif
            "currently):\n"
            "\n"
            "    meshFromShape(Shape, Fineness, SecondOrder=0,\n"
            "                         Optimize=1, AllowQuad=0)\n"
            "    meshFromShape(Shape, GrowthRate=0, SegPerEdge=0,\n"
            "                  SegPerRadius=0, SecondOrder=0, Optimize=1,\n"
            "                  AllowQuad=0)\n"
            "\n"
            "Args:\n"
            "    Shape (required, topology) - TopoShape to create mesh of.\n"
            "    LinearDeflection (required, float)\n"
            "    AngularDeflection (optional, float)\n"
            "    Segments (optional, boolean)\n"
            "    GroupColors (optional, list of (Red, Green, Blue) tuples)\n"
            "    MaxLength (required, float)\n"
            "    MaxArea (required, float)\n"
            "    LocalLength (required, float)\n"
            "    Deflection (required, float)\n"
            "    MinLength (required, float)\n"
            "    Fineness (required, integer)\n"
            "    SecondOrder (optional, integral boolean)\n"
            "    Optimize (optional, integeral boolean)\n"
            "    AllowQuad (optional, integeral boolean)\n"
            "    GrowthRate (optional, float)\n"
            "    SegPerEdge (optional, float)\n"
            "    SegPerRadius (optional, float)\n"
        );
        initialize("This module is the MeshPart module."); // register with Python
    }

    virtual ~Module() {}

private:
    virtual Py::Object invoke_method_varargs(void *method_def, const Py::Tuple &args)
    {
        try {
            return Py::ExtensionModule<Module>::invoke_method_varargs(method_def, args);
        }
        catch (const Standard_Failure &e) {
            std::string str;
            Standard_CString msg = e.GetMessageString();
            str += typeid(e).name();
            str += " ";
            if (msg) {str += msg;}
            else     {str += "No OCCT Exception Message";}
            Base::Console().Error("%s\n", str.c_str());
            throw Py::Exception(Base::BaseExceptionFreeCADError, str);
        }
        catch (const Base::Exception &e) {
            std::string str;
            str += "FreeCAD exception thrown (";
            str += e.what();
            str += ")";
            e.ReportException();
            throw Py::RuntimeError(str);
        }
        catch (const std::exception &e) {
            std::string str;
            str += "C++ exception thrown (";
            str += e.what();
            str += ")";
            Base::Console().Error("%s\n", str.c_str());
            throw Py::RuntimeError(str);
        }
    }

    Py::Object loftOnCurve(const Py::Tuple& args)
    {
        Part::TopoShapePy   *pcObject;
        PyObject *pcTopoObj,*pcListObj;
        float x=0.0f,y=0.0f,z=1.0f,size = 0.1f;

        if (!PyArg_ParseTuple(args.ptr(), "O!O(fff)f", &(Part::TopoShapePy::Type), &pcTopoObj,&pcListObj,&x,&y,&z,&size))
//      if (!PyArg_ParseTuple(args, "O!O!", &(App::TopoShapePy::Type), &pcTopoObj,&PyList_Type,&pcListObj,x,y,z,size))
            throw Py::Exception();

        pcObject = static_cast<Part::TopoShapePy*>(pcTopoObj);
        MeshCore::MeshKernel M;

        std::vector<Base::Vector3f> poly;
        auto exText( "List of Tuples of three or two floats needed as second parameter!" );

        if (!PyList_Check(pcListObj))
            throw Py::Exception(Base::BaseExceptionFreeCADError, exText);

        int nSize = PyList_Size(pcListObj);
        for (int i=0; i<nSize;++i) {
            PyObject* item = PyList_GetItem(pcListObj, i);
            if (!PyTuple_Check(item))
                throw Py::Exception(Base::BaseExceptionFreeCADError, exText);

            int nTSize = PyTuple_Size(item);
            if (nTSize != 2 && nTSize != 3)
                throw Py::Exception(Base::BaseExceptionFreeCADError, exText);

            Base::Vector3f vec(0,0,0);

            for(int l = 0; l < nTSize;l++) {
                PyObject* item2 = PyTuple_GetItem(item, l);
                if (!PyFloat_Check(item2))
                    throw Py::Exception(Base::BaseExceptionFreeCADError, exText);
                vec[l] = (float)PyFloat_AS_DOUBLE(item2);
            }
            poly.push_back(vec);
        }

        TopoDS_Shape aShape = pcObject->getTopoShapePtr()->getShape();
        // use the MeshAlgos 
        MeshPart::MeshAlgos::LoftOnCurve(M,aShape,poly,Base::Vector3f(x,y,z),size);
        return Py::asObject(new Mesh::MeshPy(new Mesh::MeshObject(M)));
    }
    Py::Object projectShapeOnMesh(const Py::Tuple& args)
    {
        PyObject *s, *m;
        double maxDist;
        if (!PyArg_ParseTuple(args.ptr(), "O!O!d", &Part::TopoShapePy::Type, &s,
                                                   &Mesh::MeshPy::Type, &m,
                                                   &maxDist))
            throw Py::Exception();
        TopoDS_Shape shape = static_cast<Part::TopoShapePy*>(s)->getTopoShapePtr()->getShape();
        const Mesh::MeshObject* mesh = static_cast<Mesh::MeshPy*>(m)->getMeshObjectPtr();
        MeshCore::MeshKernel kernel(mesh->getKernel());
        kernel.Transform(mesh->getTransform());

        MeshProjection proj(kernel);
        std::vector<MeshProjection::SplitEdge> rSplitEdges;
        proj.projectToMesh(shape, maxDist, rSplitEdges);

        Py::List list;
        for (auto it : rSplitEdges) {
            Py::Vector v(it.cPt);
            list.append(v);
        }

        return list;
    }
    Py::Object wireFromSegment(const Py::Tuple& args)
    {
        PyObject *o, *m;
        if (!PyArg_ParseTuple(args.ptr(), "O!O!", &(Mesh::MeshPy::Type), &m,&PyList_Type,&o))
            throw Py::Exception();

        Py::List list(o);
        Mesh::MeshObject* mesh = static_cast<Mesh::MeshPy*>(m)->getMeshObjectPtr();
        std::vector<unsigned long> segm;
        segm.reserve(list.size());
        for (unsigned int i=0; i<list.size(); i++) {
            segm.push_back((long)Py::Long(list[i]));
        }

        std::list<std::vector<Base::Vector3f> > bounds;
        MeshCore::MeshAlgorithm algo(mesh->getKernel());
        algo.GetFacetBorders(segm, bounds);

        Py::List wires;
        std::list<std::vector<Base::Vector3f> >::iterator bt;

        for (bt = bounds.begin(); bt != bounds.end(); ++bt) {
            BRepBuilderAPI_MakePolygon mkPoly;
            for (std::vector<Base::Vector3f>::reverse_iterator it = bt->rbegin(); it != bt->rend(); ++it) {
                mkPoly.Add(gp_Pnt(it->x,it->y,it->z));
            }
            if (mkPoly.IsDone()) {
                PyObject* wire = new Part::TopoShapeWirePy(new Part::TopoShape(mkPoly.Wire()));
                wires.append(Py::Object(wire, true));
            }
        }

        return wires;
    }
    Py::Object meshFromShape(const Py::Tuple& args, const Py::Dict& kwds)
    {
        PyObject *shape;

        static char* kwds_lindeflection[] = {"Shape", "LinearDeflection", "AngularDeflection",
                                             "Relative", "Segments", "GroupColors", NULL};
        PyErr_Clear();
        double lindeflection=0;
        double angdeflection=0.5;
        PyObject* relative = Py_False;
        PyObject* segment = Py_False;
        PyObject* groupColors = 0;
        if (PyArg_ParseTupleAndKeywords(args.ptr(), kwds.ptr(), "O!d|dO!O!O", kwds_lindeflection,
                                        &(Part::TopoShapePy::Type), &shape, &lindeflection,
                                        &angdeflection, &(PyBool_Type), &relative,
                                        &(PyBool_Type), &segment, &groupColors)) {
            MeshPart::Mesher mesher(static_cast<Part::TopoShapePy*>(shape)->getTopoShapePtr()->getShape());
            mesher.setMethod(MeshPart::Mesher::Standard);
            mesher.setDeflection(lindeflection);
            mesher.setAngularDeflection(angdeflection);
            mesher.setRegular(true);
            mesher.setRelative(PyObject_IsTrue(relative) ? true : false);
            mesher.setSegments(PyObject_IsTrue(segment) ? true : false);
            if (groupColors) {
                Py::Sequence list(groupColors);
                std::vector<uint32_t> colors;
                colors.reserve(list.size());
                for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
                    Py::Tuple t(*it);
                    Py::Float r(t[0]);
                    Py::Float g(t[1]);
                    Py::Float b(t[2]);
                    App::Color c(static_cast<float>(r),
                                 static_cast<float>(g),
                                 static_cast<float>(b));
                    colors.push_back(c.getPackedValue());
                }
                mesher.setColors(colors);
            }
            return Py::asObject(new Mesh::MeshPy(mesher.createMesh()));
        }

        static char* kwds_maxLength[] = {"Shape", "MaxLength",NULL};
        PyErr_Clear();
        double maxLength=0;
        if (PyArg_ParseTupleAndKeywords(args.ptr(), kwds.ptr(), "O!d", kwds_maxLength,
                                        &(Part::TopoShapePy::Type), &shape, &maxLength)) {
            MeshPart::Mesher mesher(static_cast<Part::TopoShapePy*>(shape)->getTopoShapePtr()->getShape());
            mesher.setMethod(MeshPart::Mesher::Mefisto);
            mesher.setMaxLength(maxLength);
            mesher.setRegular(true);
            return Py::asObject(new Mesh::MeshPy(mesher.createMesh()));
        }

        static char* kwds_maxArea[] = {"Shape", "MaxArea",NULL};
        PyErr_Clear();
        double maxArea=0;
        if (PyArg_ParseTupleAndKeywords(args.ptr(), kwds.ptr(), "O!d", kwds_maxArea,
                                        &(Part::TopoShapePy::Type), &shape, &maxArea)) {
            MeshPart::Mesher mesher(static_cast<Part::TopoShapePy*>(shape)->getTopoShapePtr()->getShape());
            mesher.setMethod(MeshPart::Mesher::Mefisto);
            mesher.setMaxArea(maxArea);
            mesher.setRegular(true);
            return Py::asObject(new Mesh::MeshPy(mesher.createMesh()));
        }

        static char* kwds_localLen[] = {"Shape", "LocalLength",NULL};
        PyErr_Clear();
        double localLen=0;
        if (PyArg_ParseTupleAndKeywords(args.ptr(), kwds.ptr(), "O!d", kwds_localLen,
                                        &(Part::TopoShapePy::Type), &shape, &localLen)) {
            MeshPart::Mesher mesher(static_cast<Part::TopoShapePy*>(shape)->getTopoShapePtr()->getShape());
            mesher.setMethod(MeshPart::Mesher::Mefisto);
            mesher.setLocalLength(localLen);
            mesher.setRegular(true);
            return Py::asObject(new Mesh::MeshPy(mesher.createMesh()));
        }

        static char* kwds_deflection[] = {"Shape", "Deflection",NULL};
        PyErr_Clear();
        double deflection=0;
        if (PyArg_ParseTupleAndKeywords(args.ptr(), kwds.ptr(), "O!d", kwds_deflection,
                                        &(Part::TopoShapePy::Type), &shape, &deflection)) {
            MeshPart::Mesher mesher(static_cast<Part::TopoShapePy*>(shape)->getTopoShapePtr()->getShape());
            mesher.setMethod(MeshPart::Mesher::Mefisto);
            mesher.setDeflection(deflection);
            mesher.setRegular(true);
            return Py::asObject(new Mesh::MeshPy(mesher.createMesh()));
        }

        static char* kwds_minmaxLen[] = {"Shape", "MinLength","MaxLength",NULL};
        PyErr_Clear();
        double minLen=0, maxLen=0;
        if (PyArg_ParseTupleAndKeywords(args.ptr(), kwds.ptr(), "O!dd", kwds_minmaxLen,
                                        &(Part::TopoShapePy::Type), &shape, &minLen, &maxLen)) {
            MeshPart::Mesher mesher(static_cast<Part::TopoShapePy*>(shape)->getTopoShapePtr()->getShape());
            mesher.setMethod(MeshPart::Mesher::Mefisto);
            mesher.setMinMaxLengths(minLen, maxLen);
            mesher.setRegular(true);
            return Py::asObject(new Mesh::MeshPy(mesher.createMesh()));
        }

#if defined (HAVE_NETGEN)
        static char* kwds_fineness[] = {"Shape", "Fineness", "SecondOrder", "Optimize", "AllowQuad",NULL};
        PyErr_Clear();
        int fineness=0, secondOrder=0, optimize=1, allowquad=0;
        if (PyArg_ParseTupleAndKeywords(args.ptr(), kwds.ptr(), "O!i|iii", kwds_fineness,
                                        &(Part::TopoShapePy::Type), &shape, &fineness,
                                        &secondOrder, &optimize, &allowquad)) {
            MeshPart::Mesher mesher(static_cast<Part::TopoShapePy*>(shape)->getTopoShapePtr()->getShape());
            mesher.setMethod(MeshPart::Mesher::Netgen);
            mesher.setFineness(fineness);
            mesher.setSecondOrder(secondOrder != 0);
            mesher.setOptimize(optimize != 0);
            mesher.setQuadAllowed(allowquad != 0);
            return Py::asObject(new Mesh::MeshPy(mesher.createMesh()));
        }

        static char* kwds_user[] = {"Shape", "GrowthRate", "SegPerEdge", "SegPerRadius", "SecondOrder", "Optimize", "AllowQuad",NULL};
        PyErr_Clear();
        double growthRate=0, nbSegPerEdge=0, nbSegPerRadius=0;
        if (PyArg_ParseTupleAndKeywords(args.ptr(), kwds.ptr(), "O!|dddiii", kwds_user,
                                        &(Part::TopoShapePy::Type), &shape,
                                        &growthRate, &nbSegPerEdge, &nbSegPerRadius,
                                        &secondOrder, &optimize, &allowquad)) {
            MeshPart::Mesher mesher(static_cast<Part::TopoShapePy*>(shape)->getTopoShapePtr()->getShape());
            mesher.setMethod(MeshPart::Mesher::Netgen);
            mesher.setGrowthRate(growthRate);
            mesher.setNbSegPerEdge(nbSegPerEdge);
            mesher.setNbSegPerRadius(nbSegPerRadius);
            mesher.setSecondOrder(secondOrder != 0);
            mesher.setOptimize(optimize != 0);
            mesher.setQuadAllowed(allowquad != 0);
            return Py::asObject(new Mesh::MeshPy(mesher.createMesh()));
        }
#endif

        PyErr_Clear();
        if (PyArg_ParseTuple(args.ptr(), "O!", &(Part::TopoShapePy::Type), &shape)) {
            MeshPart::Mesher mesher(static_cast<Part::TopoShapePy*>(shape)->getTopoShapePtr()->getShape());
#if defined (HAVE_NETGEN)
            mesher.setMethod(MeshPart::Mesher::Netgen);
#else
            mesher.setMethod(MeshPart::Mesher::Mefisto);
            mesher.setRegular(true);
#endif
            return Py::asObject(new Mesh::MeshPy(mesher.createMesh()));
        }

        throw Py::Exception(Base::BaseExceptionFreeCADError,"Wrong arguments");
    }
};

PyObject* initModule()
{
    return (new Module)->module().ptr();
}

} // namespace MeshPart
