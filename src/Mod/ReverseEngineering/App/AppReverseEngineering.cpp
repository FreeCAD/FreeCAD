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
# include <Geom_BSplineSurface.hxx>
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
#include "RegionGrowing.h"
#include "Segmentation.h"
#include "SampleConsensus.h"
#if defined(HAVE_PCL_FILTERS)
#include <pcl/filters/passthrough.h>
#include <pcl/filters/voxel_grid.h>
#include <pcl/point_types.h>
#endif

/*
Dependency of pcl components:
common: none
features: common, kdtree, octree, search, (range_image)
filters: common, kdtree, octree, sample_consenus, search
geomety: common
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
            "approxSurface(Points=,UDegree=3,VDegree=3,NbUPoles=6,NbVPoles=6,Smooth=True)\n"
            "Weight=0.1,Grad=1.0,Bend=0.0,\n"
            "Iterations=5,Correction=True,PatchFactor=1.0"
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
            "normalEstimation(Points)."
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

        static char* kwds_greedy[] = {"Points", "SearchRadius", "Mu", "KSearch",
                                      "Normals", NULL};
        if (!PyArg_ParseTupleAndKeywords(args.ptr(), kwds.ptr(), "O!d|diO", kwds_greedy,
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

        static char* kwds_poisson[] = {"Points", "KSearch", "OctreeDepth", "SolverDivide",
                                      "SamplesPerNode", "Normals", NULL};
        if (!PyArg_ParseTupleAndKeywords(args.ptr(), kwds.ptr(), "O!|iiidO", kwds_poisson,
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

        static char* kwds_view[] = {"Points", "Width", "Height", NULL};
        if (!PyArg_ParseTupleAndKeywords(args.ptr(), kwds.ptr(), "O!|ii", kwds_view,
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

        static char* kwds_greedy[] = {"Points", "KSearch", "Normals", NULL};
        if (!PyArg_ParseTupleAndKeywords(args.ptr(), kwds.ptr(), "O!|iO", kwds_greedy,
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

        static char* kwds_greedy[] = {"Points", "KSearch", "Normals", NULL};
        if (!PyArg_ParseTupleAndKeywords(args.ptr(), kwds.ptr(), "O!|iO", kwds_greedy,
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

        static char* kwds_greedy[] = {"Points", "KSearch", "Normals", NULL};
        if (!PyArg_ParseTupleAndKeywords(args.ptr(), kwds.ptr(), "O!|iO", kwds_greedy,
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

        static char* kwds_approx[] = {"Points", "Degree", "Refinement", "Iterations",
                                      "InteriorSmoothness", "InteriorWeight", "BoundarySmoothness", "BoundaryWeight", NULL};
        if (!PyArg_ParseTupleAndKeywords(args.ptr(), kwds.ptr(), "O!|iiidddd", kwds_approx,
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

        throw Py::RuntimeError("Computation of B-Spline surface failed");
    }
#endif
#if defined(HAVE_PCL_FILTERS)
    Py::Object filterVoxelGrid(const Py::Tuple& args, const Py::Dict& kwds)
    {
        PyObject *pts;
        double voxDimX = 0;
        double voxDimY = 0;
        double voxDimZ = 0;

        static char* kwds_voxel[] = {"Points", "DimX", "DimY", "DimZ", NULL};
        if (!PyArg_ParseTupleAndKeywords(args.ptr(), kwds.ptr(), "O!d|dd", kwds_voxel,
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

        static char* kwds_normals[] = {"Points", "KSearch", "SearchRadius", NULL};
        if (!PyArg_ParseTupleAndKeywords(args.ptr(), kwds.ptr(), "O!|id", kwds_normals,
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

        static char* kwds_segment[] = {"Points", "KSearch", "Normals", NULL};
        if (!PyArg_ParseTupleAndKeywords(args.ptr(), kwds.ptr(), "O!|iO", kwds_segment,
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

        static char* kwds_segment[] = {"Points", "KSearch", NULL};
        if (!PyArg_ParseTupleAndKeywords(args.ptr(), kwds.ptr(), "O!|i", kwds_segment,
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
    Py::Object sampleConsensus(const Py::Tuple& args, const Py::Dict& kwds)
    {
        PyObject *pts;

        static char* kwds_sample[] = {"Points", NULL};
        if (!PyArg_ParseTupleAndKeywords(args.ptr(), kwds.ptr(), "O!", kwds_sample,
                                        &(Points::PointsPy::Type), &pts))
            throw Py::Exception();

        Points::PointKernel* points = static_cast<Points::PointsPy*>(pts)->getPointKernelPtr();

        std::vector<float> parameters;
        SampleConsensus sample(*points);
        double probability = sample.perform(parameters);

        Py::Dict dict;
        Py::Tuple tuple(parameters.size());
        for (std::size_t i = 0; i < parameters.size(); i++)
            tuple.setItem(i, Py::Float(parameters[i]));
        dict.setItem(Py::String("Probability"), Py::Float(probability));
        dict.setItem(Py::String("Parameters"), tuple);

        return dict;
    }
#endif
};

PyObject* initModule()
{
    return (new Module)->module().ptr();
}

} // namespace Reen


/* Python entry */
PyMODINIT_FUNC initReverseEngineering()
{
    // load dependent module
    try {
        Base::Interpreter().loadModule("Part");
        Base::Interpreter().loadModule("Mesh");
    }
    catch(const Base::Exception& e) {
        PyErr_SetString(PyExc_ImportError, e.what());
        PyMOD_Return(0);
    }

    PyObject* mod = Reen::initModule();
    Base::Console().Log("Loading ReverseEngineering module... done\n");
    PyMOD_Return(mod);
}
