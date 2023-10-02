/***************************************************************************
 *   Copyright (c) 2008 Jürgen Riegel <juergen.riegel@web.de>              *
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
#include <Geom_BSplineSurface.hxx>
#include <TColgp_Array1OfPnt.hxx>
#endif

#include <Base/Console.h>
#include <Base/Converter.h>
#include <Base/GeometryPyCXX.h>
#include <Base/Interpreter.h>
#include <Base/PyWrapParseTupleAndKeywords.h>
#include <Mod/Mesh/App/MeshPy.h>
#include <Mod/Part/App/BSplineSurfacePy.h>
#include <Mod/Points/App/PointsPy.h>
#if defined(HAVE_PCL_FILTERS)
#include <pcl/filters/passthrough.h>
#include <pcl/filters/voxel_grid.h>
#include <pcl/point_types.h>
#endif

#include "ApproxSurface.h"
#include "BSplineFitting.h"
#include "RegionGrowing.h"
#include "SampleConsensus.h"
#include "Segmentation.h"
#include "SurfaceTriangulation.h"

// clang-format off
/*
Dependency of pcl components:
common: none
features: common, kdtree, octree, search, (range_image)
filters: common, kdtree, octree, sample_consenus, search
geometry: common
io: common, octree
kdtree: common
keypoints: common, features, filters, kdtree, octree, search, (range_image)
octree: common
recognition: common, features, search
registration: common, features, kdtree, sample_consensus
sample_consensus: common
search: common, kdtree, octree
segmentation: common, kdtree, octree, sample_consensus, search
surface: common, kdtree, octree, search
*/

using namespace Reen;

namespace Reen {
class Module : public Py::ExtensionModule<Module>
{
public:
    Module() : Py::ExtensionModule<Module>("ReverseEngineering")
    {
        add_keyword_method("approxSurface",&Module::approxSurface,
            "approxSurface(Points, UDegree=3, VDegree=3, NbUPoles=6, NbVPoles=6,\n"
            "Smooth=True, Weight=0.1, Grad=1.0, Bend=0.0, Curv=0.0\n"
            "Iterations=5, Correction=True, PatchFactor=1.0, UVDirs=((ux, uy, uz), (vx, vy, vz)))\n\n"
            "Points: the input data (e.g. a point cloud or mesh)\n"
            "UDegree: the degree in u parametric direction\n"
            "VDegree: the degree in v parametric direction\n"
            "NbUPoles: the number of control points in u parametric direction\n"
            "NbVPoles: the number of control points in v parametric direction\n"
            "Smooth: use energy terms to create a smooth surface\n"
            "Weight: weight of the energy terms altogether\n"
            "Grad: weight of the gradient term\n"
            "Bend: weight of the bending energy term\n"
            "Curv: weight of the deviation of curvature term\n"
            "Iterations: number of iterations\n"
            "Correction: perform a parameter correction of each iteration step\n"
            "PatchFactor: create an extended surface\n"
            "UVDirs: set the u,v parameter directions as tuple of two vectors\n"
            "        If not set then they will be determined by computing a best-fit plane\n"
        );
#if defined(HAVE_PCL_SURFACE)
        add_keyword_method("triangulate",&Module::triangulate,
            "triangulate(PointKernel,searchRadius[,mu=2.5])."
        );
        add_keyword_method("poissonReconstruction",&Module::poissonReconstruction,
            "poissonReconstruction(PointKernel)."
        );
        add_keyword_method("viewTriangulation",&Module::viewTriangulation,
            "viewTriangulation(PointKernel, width, height)."
        );
        add_keyword_method("gridProjection",&Module::gridProjection,
            "gridProjection(PointKernel)."
        );
        add_keyword_method("marchingCubesRBF",&Module::marchingCubesRBF,
            "marchingCubesRBF(PointKernel)."
        );
        add_keyword_method("marchingCubesHoppe",&Module::marchingCubesHoppe,
            "marchingCubesHoppe(PointKernel)."
        );
#endif
#if defined(HAVE_PCL_OPENNURBS)
        add_keyword_method("fitBSpline",&Module::fitBSpline,
            "fitBSpline(PointKernel)."
        );
#endif
#if defined(HAVE_PCL_FILTERS)
        add_keyword_method("filterVoxelGrid",&Module::filterVoxelGrid,
            "filterVoxelGrid(dim)."
        );
        add_keyword_method("normalEstimation",&Module::normalEstimation,
            "normalEstimation(Points,[KSearch=0, SearchRadius=0]) -> Normals\n"
            "KSearch is an int and used to search the k-nearest neighbours in\n"
            "the k-d tree. Alternatively, SearchRadius (a float) can be used\n"
            "as spatial distance to determine the neighbours of a point\n"
            "Example:\n"
            "\n"
            "import ReverseEngineering as Reen\n"
            "pts=App.ActiveDocument.ActiveObject.Points\n"
            "nor=Reen.normalEstimation(pts,KSearch=5)\n"
            "\n"
            "f=App.ActiveDocument.addObject('Points::FeaturePython','Normals')\n"
            "f.addProperty('Points::PropertyNormalList','Normal')\n"
            "f.Points=pts\n"
            "f.Normal=nor\n"
            "f.ViewObject.Proxy=0\n"
            "f.ViewObject.DisplayMode=1\n"
        );
#endif
#if defined(HAVE_PCL_SEGMENTATION)
        add_keyword_method("regionGrowingSegmentation",&Module::regionGrowingSegmentation,
            "regionGrowingSegmentation()."
        );
        add_keyword_method("featureSegmentation",&Module::featureSegmentation,
            "featureSegmentation()."
        );
#endif
#if defined(HAVE_PCL_SAMPLE_CONSENSUS)
        add_keyword_method("sampleConsensus",&Module::sampleConsensus,
            "sampleConsensus()."
        );
#endif
        initialize("This module is the ReverseEngineering module."); // register with Python
    }

private:
    Py::Object approxSurface(const Py::Tuple& args, const Py::Dict& kwds)
    {
        PyObject *o;
        PyObject *uvdirs = nullptr;
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

        static const std::array<const char *, 15> kwds_approx{"Points", "UDegree", "VDegree", "NbUPoles", "NbVPoles",
                                                              "Smooth", "Weight", "Grad", "Bend", "Curv", "Iterations",
                                                              "Correction", "PatchFactor", "UVDirs", nullptr};
        if (!Base::Wrapped_ParseTupleAndKeywords(args.ptr(), kwds.ptr(), "O|iiiiO!ddddiO!dO!", kwds_approx,
                                                 &o, &uDegree, &vDegree, &uPoles, &vPoles,
                                                 &PyBool_Type, &smooth, &weight, &grad, &bend, &curv,
                                                 &iteration, &PyBool_Type, &correction, &factor,
                                                 &PyTuple_Type, &uvdirs)) {
            throw Py::Exception();
        }

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
            else if (PyObject_TypeCheck(o, &(Mesh::MeshPy::Type))) {
                const Mesh::MeshObject* mesh = static_cast<Mesh::MeshPy*>(o)->getMeshObjectPtr();
                const MeshCore::MeshPointArray& points = mesh->getKernel().GetPoints();
                pts.insert(pts.begin(), points.begin(), points.end());
            }
            else {
                Py::Sequence l(o);
                pts.reserve(l.size());
                for (Py::Sequence::iterator it = l.begin(); it != l.end(); ++it) {
                    Py::Tuple t(*it);
                    pts.emplace_back(
                        (float)Py::Float(t.getItem(0)),
                        (float)Py::Float(t.getItem(1)),
                        (float)Py::Float(t.getItem(2))
                    );
                }
            }

            TColgp_Array1OfPnt clPoints(0, pts.size()-1);
            if (clPoints.Length() < uPoles * vPoles) {
                throw Py::ValueError("Too less data points for the specified number of poles");
            }

            int index=0;
            for (const auto & pt : pts) {
                clPoints(index++) = gp_Pnt(pt.x, pt.y, pt.z);
            }

            Reen::BSplineParameterCorrection pc(uOrder,vOrder,uPoles,vPoles);
            Handle(Geom_BSplineSurface) hSurf;

            if (uvdirs) {
                Py::Tuple t(uvdirs);
                Base::Vector3d u = Py::Vector(t.getItem(0)).toVector();
                Base::Vector3d v = Py::Vector(t.getItem(1)).toVector();
                pc.SetUV(u, v);
            }
            pc.EnableSmoothing(Base::asBoolean(smooth), weight, grad, bend, curv);
            hSurf = pc.CreateSurface(clPoints, iteration, Base::asBoolean(correction), factor);
            if (!hSurf.IsNull()) {
                return Py::asObject(new Part::BSplineSurfacePy(new Part::GeomBSplineSurface(hSurf)));
            }

            throw Py::RuntimeError("Computation of B-spline surface failed");
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
    /*
import ReverseEngineering as Reen
import Points
import Mesh
import random

r=random.Random()

p=Points.Points()
pts=[]
for i in range(21):
  for j in range(21):
    pts.append(App.Vector(i,j,r.gauss(5,0.05)))

p.addPoints(pts)
m=Reen.triangulate(Points=p,SearchRadius=2.2)
Mesh.show(m)
    */
    Py::Object triangulate(const Py::Tuple& args, const Py::Dict& kwds)
    {
        PyObject *pts;
        double searchRadius;
        PyObject *vec = 0;
        int ksearch=5;
        double mu=2.5;

        static const std::array<const char*,6> kwds_greedy {"Points", "SearchRadius", "Mu", "KSearch",
                                      "Normals", NULL};
        if (!Base::Wrapped_ParseTupleAndKeywords(args.ptr(), kwds.ptr(), "O!d|diO", kwds_greedy,
                                        &(Points::PointsPy::Type), &pts,
                                        &searchRadius, &mu, &ksearch, &vec))
            throw Py::Exception();

        Points::PointKernel* points = static_cast<Points::PointsPy*>(pts)->getPointKernelPtr();

        Mesh::MeshObject* mesh = new Mesh::MeshObject();
        SurfaceTriangulation tria(*points, *mesh);
        tria.setMu(mu);
        tria.setSearchRadius(searchRadius);
        if (vec) {
            Py::Sequence list(vec);
            std::vector<Base::Vector3f> normals;
            normals.reserve(list.size());
            for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
                Base::Vector3d v = Py::Vector(*it).toVector();
                normals.push_back(Base::convertTo<Base::Vector3f>(v));
            }
            tria.perform(normals);
        }
        else {
            tria.perform(ksearch);
        }

        return Py::asObject(new Mesh::MeshPy(mesh));
    }
    Py::Object poissonReconstruction(const Py::Tuple& args, const Py::Dict& kwds)
    {
        PyObject *pts;
        PyObject *vec = 0;
        int ksearch=5;
        int octreeDepth=-1;
        int solverDivide=-1;
        double samplesPerNode=-1.0;

        static const std::array<const char*,7> kwds_poisson {"Points", "KSearch", "OctreeDepth", "SolverDivide",
                                      "SamplesPerNode", "Normals", NULL};
        if (!Base::Wrapped_ParseTupleAndKeywords(args.ptr(), kwds.ptr(), "O!|iiidO", kwds_poisson,
                                        &(Points::PointsPy::Type), &pts,
                                        &ksearch, &octreeDepth, &solverDivide, &samplesPerNode, &vec))
            throw Py::Exception();

        Points::PointKernel* points = static_cast<Points::PointsPy*>(pts)->getPointKernelPtr();

        Mesh::MeshObject* mesh = new Mesh::MeshObject();
        Reen::PoissonReconstruction poisson(*points, *mesh);
        poisson.setDepth(octreeDepth);
        poisson.setSolverDivide(solverDivide);
        poisson.setSamplesPerNode(samplesPerNode);
        if (vec) {
            Py::Sequence list(vec);
            std::vector<Base::Vector3f> normals;
            normals.reserve(list.size());
            for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
                Base::Vector3d v = Py::Vector(*it).toVector();
                normals.push_back(Base::convertTo<Base::Vector3f>(v));
            }
            poisson.perform(normals);
        }
        else {
            poisson.perform(ksearch);
        }

        return Py::asObject(new Mesh::MeshPy(mesh));
    }
    /*
import ReverseEngineering as Reen
import Points
import Mesh
import random
import math

r=random.Random()

p=Points.Points()
pts=[]
for i in range(21):
  for j in range(21):
    pts.append(App.Vector(i,j,r.random()))

p.addPoints(pts)
m=Reen.viewTriangulation(p,21,21)
Mesh.show(m)

def boxmueller():
  r1,r2=random.random(),random.random()
  return math.sqrt(-2*math.log(r1))*math.cos(2*math.pi*r2)

p=Points.Points()
pts=[]
for i in range(21):
  for j in range(21):
    pts.append(App.Vector(i,j,r.gauss(5,0.05)))

p.addPoints(pts)
m=Reen.viewTriangulation(p,21,21)
Mesh.show(m)
    */
    Py::Object viewTriangulation(const Py::Tuple& args, const Py::Dict& kwds)
    {
        PyObject *pts;
        int width;
        int height;

        static const std::array<const char*,4> kwds_view {"Points", "Width", "Height", NULL};
        if (!Base::Wrapped_ParseTupleAndKeywords(args.ptr(), kwds.ptr(), "O!|ii", kwds_view,
                                        &(Points::PointsPy::Type), &pts,
                                        &width, &height))
            throw Py::Exception();

        Points::PointKernel* points = static_cast<Points::PointsPy*>(pts)->getPointKernelPtr();

        try {
            Mesh::MeshObject* mesh = new Mesh::MeshObject();
            ImageTriangulation view(width, height, *points, *mesh);
            view.perform();

            return Py::asObject(new Mesh::MeshPy(mesh));
        }
        catch (const Base::Exception& e) {
            throw Py::RuntimeError(e.what());
        }
    }
    Py::Object gridProjection(const Py::Tuple& args, const Py::Dict& kwds)
    {
        PyObject *pts;
        PyObject *vec = 0;
        int ksearch=5;

        static const std::array<const char*,4> kwds_greedy {"Points", "KSearch", "Normals", NULL};
        if (!Base::Wrapped_ParseTupleAndKeywords(args.ptr(), kwds.ptr(), "O!|iO", kwds_greedy,
                                        &(Points::PointsPy::Type), &pts,
                                        &ksearch, &vec))
            throw Py::Exception();

        Points::PointKernel* points = static_cast<Points::PointsPy*>(pts)->getPointKernelPtr();

        Mesh::MeshObject* mesh = new Mesh::MeshObject();
        GridReconstruction tria(*points, *mesh);
        if (vec) {
            Py::Sequence list(vec);
            std::vector<Base::Vector3f> normals;
            normals.reserve(list.size());
            for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
                Base::Vector3d v = Py::Vector(*it).toVector();
                normals.push_back(Base::convertTo<Base::Vector3f>(v));
            }
            tria.perform(normals);
        }
        else {
            tria.perform(ksearch);
        }

        return Py::asObject(new Mesh::MeshPy(mesh));
    }
    Py::Object marchingCubesRBF(const Py::Tuple& args, const Py::Dict& kwds)
    {
        PyObject *pts;
        PyObject *vec = 0;
        int ksearch=5;

        static const std::array<const char*,4> kwds_greedy {"Points", "KSearch", "Normals", NULL};
        if (!Base::Wrapped_ParseTupleAndKeywords(args.ptr(), kwds.ptr(), "O!|iO", kwds_greedy,
                                        &(Points::PointsPy::Type), &pts,
                                        &ksearch, &vec))
            throw Py::Exception();

        Points::PointKernel* points = static_cast<Points::PointsPy*>(pts)->getPointKernelPtr();

        Mesh::MeshObject* mesh = new Mesh::MeshObject();
        MarchingCubesRBF tria(*points, *mesh);
        if (vec) {
            Py::Sequence list(vec);
            std::vector<Base::Vector3f> normals;
            normals.reserve(list.size());
            for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
                Base::Vector3d v = Py::Vector(*it).toVector();
                normals.push_back(Base::convertTo<Base::Vector3f>(v));
            }
            tria.perform(normals);
        }
        else {
            tria.perform(ksearch);
        }

        return Py::asObject(new Mesh::MeshPy(mesh));
    }
    /*
import ReverseEngineering as Reen
import Points
import Mesh
import random

r=random.Random()

p=Points.Points()
pts=[]
for i in range(21):
  for j in range(21):
    pts.append(App.Vector(i,j,r.gauss(5,0.05)))

p.addPoints(pts)
m=Reen.marchingCubesHoppe(Points=p)
Mesh.show(m)
    */
    Py::Object marchingCubesHoppe(const Py::Tuple& args, const Py::Dict& kwds)
    {
        PyObject *pts;
        PyObject *vec = 0;
        int ksearch=5;

        static const std::array<const char*,4> kwds_greedy {"Points", "KSearch", "Normals", NULL};
        if (!Base::Wrapped_ParseTupleAndKeywords(args.ptr(), kwds.ptr(), "O!|iO", kwds_greedy,
                                        &(Points::PointsPy::Type), &pts,
                                        &ksearch, &vec))
            throw Py::Exception();

        Points::PointKernel* points = static_cast<Points::PointsPy*>(pts)->getPointKernelPtr();

        Mesh::MeshObject* mesh = new Mesh::MeshObject();
        MarchingCubesHoppe tria(*points, *mesh);
        if (vec) {
            Py::Sequence list(vec);
            std::vector<Base::Vector3f> normals;
            normals.reserve(list.size());
            for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
                Base::Vector3d v = Py::Vector(*it).toVector();
                normals.push_back(Base::convertTo<Base::Vector3f>(v));
            }
            tria.perform(normals);
        }
        else {
            tria.perform(ksearch);
        }

        return Py::asObject(new Mesh::MeshPy(mesh));
    }
#endif
#if defined(HAVE_PCL_OPENNURBS)
    Py::Object fitBSpline(const Py::Tuple& args, const Py::Dict& kwds)
    {
        PyObject *pts;
        int degree = 2;
        int refinement = 4;
        int iterations = 10;
        double interiorSmoothness = 0.2;
        double interiorWeight = 1.0;
        double boundarySmoothness = 0.2;
        double boundaryWeight = 0.0;

        static const std::array<const char*,9> kwds_approx {"Points", "Degree", "Refinement", "Iterations",
                                      "InteriorSmoothness", "InteriorWeight", "BoundarySmoothness", "BoundaryWeight", NULL};
        if (!Base::Wrapped_ParseTupleAndKeywords(args.ptr(), kwds.ptr(), "O!|iiidddd", kwds_approx,
                                        &(Points::PointsPy::Type), &pts,
                                        &degree, &refinement, &iterations,
                                        &interiorSmoothness, &interiorWeight,
                                        &boundarySmoothness, &boundaryWeight))
            throw Py::Exception();

        Points::PointKernel* points = static_cast<Points::PointsPy*>(pts)->getPointKernelPtr();

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

        throw Py::RuntimeError("Computation of B-spline surface failed");
    }
#endif
#if defined(HAVE_PCL_FILTERS)
    Py::Object filterVoxelGrid(const Py::Tuple& args, const Py::Dict& kwds)
    {
        PyObject *pts;
        double voxDimX = 0;
        double voxDimY = 0;
        double voxDimZ = 0;

        static const std::array<const char*,5>  kwds_voxel {"Points", "DimX", "DimY", "DimZ", NULL};
        if (!Base::Wrapped_ParseTupleAndKeywords(args.ptr(), kwds.ptr(), "O!d|dd", kwds_voxel,
                                        &(Points::PointsPy::Type), &pts,
                                        &voxDimX, &voxDimY, &voxDimZ))
            throw Py::Exception();

        if (voxDimY == 0)
            voxDimY = voxDimX;

        if (voxDimZ == 0)
            voxDimZ = voxDimX;

        Points::PointKernel* points = static_cast<Points::PointsPy*>(pts)->getPointKernelPtr();

        pcl::PointCloud<pcl::PointXYZ>::Ptr cloud (new pcl::PointCloud<pcl::PointXYZ>);
        cloud->reserve(points->size());
        for (Points::PointKernel::const_iterator it = points->begin(); it != points->end(); ++it) {
            cloud->push_back(pcl::PointXYZ(it->x, it->y, it->z));
        }

        // Create the filtering object
        pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_downSmpl (new pcl::PointCloud<pcl::PointXYZ>);
        pcl::VoxelGrid<pcl::PointXYZ> voxG;
        voxG.setInputCloud (cloud);
        voxG.setLeafSize (voxDimX, voxDimY, voxDimZ);
        voxG.filter (*cloud_downSmpl);

        Points::PointKernel* points_sample = new Points::PointKernel();
        points_sample->reserve(cloud_downSmpl->size());
        for (pcl::PointCloud<pcl::PointXYZ>::const_iterator it = cloud_downSmpl->begin();it!=cloud_downSmpl->end();++it) {
            points_sample->push_back(Base::Vector3d(it->x,it->y,it->z));
        }

        return Py::asObject(new Points::PointsPy(points_sample));
    }
#endif
#if defined(HAVE_PCL_FILTERS)
    Py::Object normalEstimation(const Py::Tuple& args, const Py::Dict& kwds)
    {
        PyObject *pts;
        int ksearch=0;
        double searchRadius=0;

        static const std::array<const char*,4> kwds_normals {"Points", "KSearch", "SearchRadius", NULL};
        if (!Base::Wrapped_ParseTupleAndKeywords(args.ptr(), kwds.ptr(), "O!|id", kwds_normals,
                                        &(Points::PointsPy::Type), &pts,
                                        &ksearch, &searchRadius))
            throw Py::Exception();

        Points::PointKernel* points = static_cast<Points::PointsPy*>(pts)->getPointKernelPtr();

        std::vector<Base::Vector3d> normals;
        NormalEstimation estimate(*points);
        estimate.setKSearch(ksearch);
        estimate.setSearchRadius(searchRadius);
        estimate.perform(normals);

        Py::List list;
        for (std::vector<Base::Vector3d>::iterator it = normals.begin(); it != normals.end(); ++it) {
            list.append(Py::Vector(*it));
        }

        return list;
    }
#endif
#if defined(HAVE_PCL_SEGMENTATION)
    Py::Object regionGrowingSegmentation(const Py::Tuple& args, const Py::Dict& kwds)
    {
        PyObject *pts;
        PyObject *vec = 0;
        int ksearch=5;

        static const std::array<const char*,4> kwds_segment {"Points", "KSearch", "Normals", NULL};
        if (!Base::Wrapped_ParseTupleAndKeywords(args.ptr(), kwds.ptr(), "O!|iO", kwds_segment,
                                        &(Points::PointsPy::Type), &pts,
                                        &ksearch, &vec))
            throw Py::Exception();

        Points::PointKernel* points = static_cast<Points::PointsPy*>(pts)->getPointKernelPtr();

        std::list<std::vector<int> > clusters;
        RegionGrowing segm(*points, clusters);
        if (vec) {
            Py::Sequence list(vec);
            std::vector<Base::Vector3f> normals;
            normals.reserve(list.size());
            for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
                Base::Vector3d v = Py::Vector(*it).toVector();
                normals.push_back(Base::convertTo<Base::Vector3f>(v));
            }
            segm.perform(normals);
        }
        else {
            segm.perform(ksearch);
        }

        Py::List lists;
        for (std::list<std::vector<int> >::iterator it = clusters.begin(); it != clusters.end(); ++it) {
            Py::Tuple tuple(it->size());
            for (std::size_t i = 0; i < it->size(); i++) {
                tuple.setItem(i, Py::Long((*it)[i]));
            }
            lists.append(tuple);
        }

        return lists;
    }
    Py::Object featureSegmentation(const Py::Tuple& args, const Py::Dict& kwds)
    {
        PyObject *pts;
        int ksearch=5;

        static const std::array<const char*,3> kwds_segment {"Points", "KSearch", NULL};
        if (!Base::Wrapped_ParseTupleAndKeywords(args.ptr(), kwds.ptr(), "O!|i", kwds_segment,
                                        &(Points::PointsPy::Type), &pts, &ksearch))
            throw Py::Exception();

        Points::PointKernel* points = static_cast<Points::PointsPy*>(pts)->getPointKernelPtr();

        std::list<std::vector<int> > clusters;
        Segmentation segm(*points, clusters);
        segm.perform(ksearch);

        Py::List lists;
        for (std::list<std::vector<int> >::iterator it = clusters.begin(); it != clusters.end(); ++it) {
            Py::Tuple tuple(it->size());
            for (std::size_t i = 0; i < it->size(); i++) {
                tuple.setItem(i, Py::Long((*it)[i]));
            }
            lists.append(tuple);
        }

        return lists;
    }
#endif
#if defined(HAVE_PCL_SAMPLE_CONSENSUS)
    /*
import ReverseEngineering as reen
import Points
import Part

p = App.ActiveDocument.Points.Points
data = p.Points
n = reen.normalEstimation(p, 10)

model = reen.sampleConsensus(SacModel="Plane", Points=p)
indices = model["Model"]
param = model["Parameters"]

plane = Part.Plane()
plane.Axis = param[0:3]
plane.Position = -plane.Axis * param[3]

np = Points.Points()
np.addPoints([data[i] for i in indices])
Points.show(np)

# sort in descending order
indices = list(indices)
indices.sort(reverse=True)

# remove points of segment
for i in indices:
    del data[i]
    del n[i]

p = Points.Points()
p.addPoints(data)
model = reen.sampleConsensus(SacModel="Cylinder", Points=p, Normals=n)
indices = model["Model"]

np = Points.Points()
np.addPoints([data[i] for i in indices])
Points.show(np)
    */
    Py::Object sampleConsensus(const Py::Tuple& args, const Py::Dict& kwds)
    {
        PyObject *pts;
        PyObject *vec = nullptr;
        const char* sacModelType = nullptr;

        static const std::array<const char*,4> kwds_sample {"SacModel", "Points", "Normals", NULL};
        if (!Base::Wrapped_ParseTupleAndKeywords(args.ptr(), kwds.ptr(), "sO!|O", kwds_sample,
                                        &sacModelType, &(Points::PointsPy::Type), &pts, &vec))
            throw Py::Exception();

        Points::PointKernel* points = static_cast<Points::PointsPy*>(pts)->getPointKernelPtr();
        std::vector<Base::Vector3d> normals;
        if (vec) {
            Py::Sequence list(vec);
            normals.reserve(list.size());
            for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
                Base::Vector3d v = Py::Vector(*it).toVector();
                normals.push_back(v);
            }
        }

        SampleConsensus::SacModel sacModel = SampleConsensus::SACMODEL_PLANE;
        if (sacModelType) {
            if (strcmp(sacModelType, "Cylinder") == 0)
                sacModel = SampleConsensus::SACMODEL_CYLINDER;
            else if (strcmp(sacModelType, "Sphere") == 0)
                sacModel = SampleConsensus::SACMODEL_SPHERE;
            else if (strcmp(sacModelType, "Cone") == 0)
                sacModel = SampleConsensus::SACMODEL_CONE;
        }

        std::vector<float> parameters;
        SampleConsensus sample(sacModel, *points, normals);
        std::vector<int> model;
        double probability = sample.perform(parameters, model);

        Py::Dict dict;
        Py::Tuple tuple(parameters.size());
        for (std::size_t i = 0; i < parameters.size(); i++)
            tuple.setItem(i, Py::Float(parameters[i]));
        Py::Tuple data(model.size());
        for (std::size_t i = 0; i < model.size(); i++)
            data.setItem(i, Py::Long(model[i]));
        dict.setItem(Py::String("Probability"), Py::Float(probability));
        dict.setItem(Py::String("Parameters"), tuple);
        dict.setItem(Py::String("Model"), data);

        return dict;
    }
#endif
};

PyObject* initModule()
{
    return Base::Interpreter().addModule(new Module);
}

} // namespace Reen


/* Python entry */
PyMOD_INIT_FUNC(ReverseEngineering)
{
    // load dependent module
    try {
        Base::Interpreter().loadModule("Part");
        Base::Interpreter().loadModule("Mesh");
    }
    catch(const Base::Exception& e) {
        PyErr_SetString(PyExc_ImportError, e.what());
        PyMOD_Return(nullptr);
    }

    PyObject* mod = Reen::initModule();
    Base::Console().Log("Loading ReverseEngineering module... done\n");
    PyMOD_Return(mod);
}
// clang-format on
