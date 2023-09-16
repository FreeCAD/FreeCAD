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

#include <Base/Exception.h>
#include <Mod/Mesh/App/Core/Algorithm.h>
#include <Mod/Mesh/App/Core/Elements.h>
#include <Mod/Mesh/App/Core/MeshKernel.h>
#include <Mod/Mesh/App/Mesh.h>
#include <Mod/Points/App/Points.h>

#include "SurfaceTriangulation.h"


// http://svn.pointclouds.org/pcl/tags/pcl-1.5.1/test/
#if defined(HAVE_PCL_SURFACE)
#include <boost/math/special_functions/fpclassify.hpp>
#include <boost/random.hpp>
#include <pcl/common/common.h>
#include <pcl/common/io.h>
#include <pcl/features/normal_3d.h>
#include <pcl/pcl_config.h>
#include <pcl/point_traits.h>
#include <pcl/point_types.h>
#include <pcl/surface/ear_clipping.h>
#include <pcl/surface/gp3.h>
#include <pcl/surface/grid_projection.h>
#include <pcl/surface/marching_cubes_hoppe.h>
#include <pcl/surface/marching_cubes_rbf.h>
#include <pcl/surface/mls.h>
#include <pcl/surface/organized_fast_mesh.h>
#include <pcl/surface/poisson.h>

#ifndef PCL_REVISION_VERSION
#define PCL_REVISION_VERSION 0
#endif

using namespace pcl;
using namespace pcl::io;
using namespace std;
using namespace Reen;

// See
// http://www.ics.uci.edu/~gopi/PAPERS/Euro00.pdf
// http://www.ics.uci.edu/~gopi/PAPERS/CGMV.pdf
SurfaceTriangulation::SurfaceTriangulation(const Points::PointKernel& pts, Mesh::MeshObject& mesh)
    : myPoints(pts)
    , myMesh(mesh)
    , mu(0)
    , searchRadius(0)
{}

void SurfaceTriangulation::perform(int ksearch)
{
    PointCloud<PointXYZ>::Ptr cloud(new PointCloud<PointXYZ>);
    PointCloud<PointNormal>::Ptr cloud_with_normals(new PointCloud<PointNormal>);
    search::KdTree<PointXYZ>::Ptr tree;
    search::KdTree<PointNormal>::Ptr tree2;

    cloud->reserve(myPoints.size());
    for (Points::PointKernel::const_iterator it = myPoints.begin(); it != myPoints.end(); ++it) {
        if (!boost::math::isnan(it->x) && !boost::math::isnan(it->y)
            && !boost::math::isnan(it->z)) {
            cloud->push_back(PointXYZ(it->x, it->y, it->z));
        }
    }

    // Create search tree
    tree.reset(new search::KdTree<PointXYZ>(false));
    tree->setInputCloud(cloud);

    // Normal estimation
    NormalEstimation<PointXYZ, Normal> n;
    PointCloud<Normal>::Ptr normals(new PointCloud<Normal>());
    n.setInputCloud(cloud);
    // n.setIndices (indices[B);
    n.setSearchMethod(tree);
    n.setKSearch(ksearch);
    n.compute(*normals);

    // Concatenate XYZ and normal information
    pcl::concatenateFields(*cloud, *normals, *cloud_with_normals);

    // Create search tree
    tree2.reset(new search::KdTree<PointNormal>);
    tree2->setInputCloud(cloud_with_normals);

    // Init objects
    GreedyProjectionTriangulation<PointNormal> gp3;

    // Set parameters
    gp3.setInputCloud(cloud_with_normals);
    gp3.setSearchMethod(tree2);
    gp3.setSearchRadius(searchRadius);
    gp3.setMu(mu);
    gp3.setMaximumNearestNeighbors(100);
    gp3.setMaximumSurfaceAngle(M_PI / 4);  // 45 degrees
    gp3.setMinimumAngle(M_PI / 18);        // 10 degrees
    gp3.setMaximumAngle(2 * M_PI / 3);     // 120 degrees
    gp3.setNormalConsistency(false);
    gp3.setConsistentVertexOrdering(true);

    // Reconstruct
    PolygonMesh mesh;
    gp3.reconstruct(mesh);

    MeshConversion::convert(mesh, myMesh);

    // Additional vertex information
    // std::vector<int> parts = gp3.getPartIDs();
    // std::vector<int> states = gp3.getPointStates();
}

void SurfaceTriangulation::perform(const std::vector<Base::Vector3f>& normals)
{
    if (myPoints.size() != normals.size()) {
        throw Base::RuntimeError("Number of points doesn't match with number of normals");
    }

    PointCloud<PointNormal>::Ptr cloud_with_normals(new PointCloud<PointNormal>);
    search::KdTree<PointNormal>::Ptr tree;

    cloud_with_normals->reserve(myPoints.size());
    std::size_t num_points = myPoints.size();
    const std::vector<Base::Vector3f>& points = myPoints.getBasicPoints();
    for (std::size_t index = 0; index < num_points; index++) {
        const Base::Vector3f& p = points[index];
        const Base::Vector3f& n = normals[index];
        if (!boost::math::isnan(p.x) && !boost::math::isnan(p.y) && !boost::math::isnan(p.z)) {
            PointNormal pn;
            pn.x = p.x;
            pn.y = p.y;
            pn.z = p.z;
            pn.normal_x = n.x;
            pn.normal_y = n.y;
            pn.normal_z = n.z;
            cloud_with_normals->push_back(pn);
        }
    }

    // Create search tree
    tree.reset(new search::KdTree<PointNormal>);
    tree->setInputCloud(cloud_with_normals);

    // Init objects
    GreedyProjectionTriangulation<PointNormal> gp3;

    // Set parameters
    gp3.setInputCloud(cloud_with_normals);
    gp3.setSearchMethod(tree);
    gp3.setSearchRadius(searchRadius);
    gp3.setMu(mu);
    gp3.setMaximumNearestNeighbors(100);
    gp3.setMaximumSurfaceAngle(M_PI / 4);  // 45 degrees
    gp3.setMinimumAngle(M_PI / 18);        // 10 degrees
    gp3.setMaximumAngle(2 * M_PI / 3);     // 120 degrees
    gp3.setNormalConsistency(true);
    gp3.setConsistentVertexOrdering(true);

    // Reconstruct
    PolygonMesh mesh;
    gp3.reconstruct(mesh);

    MeshConversion::convert(mesh, myMesh);

    // Additional vertex information
    // std::vector<int> parts = gp3.getPartIDs();
    // std::vector<int> states = gp3.getPointStates();
}

// ----------------------------------------------------------------------------

// See
// http://www.cs.jhu.edu/~misha/Code/PoissonRecon/Version8.0/
PoissonReconstruction::PoissonReconstruction(const Points::PointKernel& pts, Mesh::MeshObject& mesh)
    : myPoints(pts)
    , myMesh(mesh)
    , depth(-1)
    , solverDivide(-1)
    , samplesPerNode(-1.0f)
{}

void PoissonReconstruction::perform(int ksearch)
{
    PointCloud<PointXYZ>::Ptr cloud(new PointCloud<PointXYZ>);
    PointCloud<PointNormal>::Ptr cloud_with_normals(new PointCloud<PointNormal>);
    search::KdTree<PointXYZ>::Ptr tree;
    search::KdTree<PointNormal>::Ptr tree2;

    cloud->reserve(myPoints.size());
    for (Points::PointKernel::const_iterator it = myPoints.begin(); it != myPoints.end(); ++it) {
        if (!boost::math::isnan(it->x) && !boost::math::isnan(it->y)
            && !boost::math::isnan(it->z)) {
            cloud->push_back(PointXYZ(it->x, it->y, it->z));
        }
    }

    // Create search tree
    tree.reset(new search::KdTree<PointXYZ>(false));
    tree->setInputCloud(cloud);

    // Normal estimation
    NormalEstimation<PointXYZ, Normal> n;
    PointCloud<Normal>::Ptr normals(new PointCloud<Normal>());
    n.setInputCloud(cloud);
    // n.setIndices (indices[B);
    n.setSearchMethod(tree);
    n.setKSearch(ksearch);
    n.compute(*normals);

    // Concatenate XYZ and normal information
    pcl::concatenateFields(*cloud, *normals, *cloud_with_normals);

    // Create search tree
    tree2.reset(new search::KdTree<PointNormal>);
    tree2->setInputCloud(cloud_with_normals);

    // Init objects
    Poisson<PointNormal> poisson;

    // Set parameters
    poisson.setInputCloud(cloud_with_normals);
    poisson.setSearchMethod(tree2);
    if (depth >= 1) {
        poisson.setDepth(depth);
    }
    if (solverDivide >= 1) {
        poisson.setSolverDivide(solverDivide);
    }
    if (samplesPerNode >= 1.0f) {
        poisson.setSamplesPerNode(samplesPerNode);
    }

    // Reconstruct
    PolygonMesh mesh;
    poisson.reconstruct(mesh);

    MeshConversion::convert(mesh, myMesh);
}

void PoissonReconstruction::perform(const std::vector<Base::Vector3f>& normals)
{
    if (myPoints.size() != normals.size()) {
        throw Base::RuntimeError("Number of points doesn't match with number of normals");
    }

    PointCloud<PointNormal>::Ptr cloud_with_normals(new PointCloud<PointNormal>);
    search::KdTree<PointNormal>::Ptr tree;

    cloud_with_normals->reserve(myPoints.size());
    std::size_t num_points = myPoints.size();
    const std::vector<Base::Vector3f>& points = myPoints.getBasicPoints();
    for (std::size_t index = 0; index < num_points; index++) {
        const Base::Vector3f& p = points[index];
        const Base::Vector3f& n = normals[index];
        if (!boost::math::isnan(p.x) && !boost::math::isnan(p.y) && !boost::math::isnan(p.z)) {
            PointNormal pn;
            pn.x = p.x;
            pn.y = p.y;
            pn.z = p.z;
            pn.normal_x = n.x;
            pn.normal_y = n.y;
            pn.normal_z = n.z;
            cloud_with_normals->push_back(pn);
        }
    }

    // Create search tree
    tree.reset(new search::KdTree<PointNormal>);
    tree->setInputCloud(cloud_with_normals);

    // Init objects
    Poisson<PointNormal> poisson;

    // Set parameters
    poisson.setInputCloud(cloud_with_normals);
    poisson.setSearchMethod(tree);
    if (depth >= 1) {
        poisson.setDepth(depth);
    }
    if (solverDivide >= 1) {
        poisson.setSolverDivide(solverDivide);
    }
    if (samplesPerNode >= 1.0f) {
        poisson.setSamplesPerNode(samplesPerNode);
    }

    // Reconstruct
    PolygonMesh mesh;
    poisson.reconstruct(mesh);

    MeshConversion::convert(mesh, myMesh);
}

// ----------------------------------------------------------------------------

GridReconstruction::GridReconstruction(const Points::PointKernel& pts, Mesh::MeshObject& mesh)
    : myPoints(pts)
    , myMesh(mesh)
{}

void GridReconstruction::perform(int ksearch)
{
    PointCloud<PointXYZ>::Ptr cloud(new PointCloud<PointXYZ>);
    PointCloud<PointNormal>::Ptr cloud_with_normals(new PointCloud<PointNormal>);
    search::KdTree<PointXYZ>::Ptr tree;
    search::KdTree<PointNormal>::Ptr tree2;

    cloud->reserve(myPoints.size());
    for (Points::PointKernel::const_iterator it = myPoints.begin(); it != myPoints.end(); ++it) {
        if (!boost::math::isnan(it->x) && !boost::math::isnan(it->y)
            && !boost::math::isnan(it->z)) {
            cloud->push_back(PointXYZ(it->x, it->y, it->z));
        }
    }

    // Create search tree
    tree.reset(new search::KdTree<PointXYZ>(false));
    tree->setInputCloud(cloud);

    // Normal estimation
    NormalEstimation<PointXYZ, Normal> n;
    PointCloud<Normal>::Ptr normals(new PointCloud<Normal>());
    n.setInputCloud(cloud);
    // n.setIndices (indices[B);
    n.setSearchMethod(tree);
    n.setKSearch(ksearch);
    n.compute(*normals);

    // Concatenate XYZ and normal information
    pcl::concatenateFields(*cloud, *normals, *cloud_with_normals);

    // Create search tree
    tree2.reset(new search::KdTree<PointNormal>);
    tree2->setInputCloud(cloud_with_normals);

    // Init objects
    GridProjection<PointNormal> grid;

    // Set parameters
    grid.setResolution(0.005);
    grid.setPaddingSize(3);
    grid.setNearestNeighborNum(100);
    grid.setMaxBinarySearchLevel(10);
    grid.setInputCloud(cloud_with_normals);
    grid.setSearchMethod(tree2);

    // Reconstruct
    PolygonMesh mesh;
    grid.reconstruct(mesh);

    MeshConversion::convert(mesh, myMesh);
}

void GridReconstruction::perform(const std::vector<Base::Vector3f>& normals)
{
    if (myPoints.size() != normals.size()) {
        throw Base::RuntimeError("Number of points doesn't match with number of normals");
    }

    PointCloud<PointNormal>::Ptr cloud_with_normals(new PointCloud<PointNormal>);
    search::KdTree<PointNormal>::Ptr tree;

    cloud_with_normals->reserve(myPoints.size());
    std::size_t num_points = myPoints.size();
    const std::vector<Base::Vector3f>& points = myPoints.getBasicPoints();
    for (std::size_t index = 0; index < num_points; index++) {
        const Base::Vector3f& p = points[index];
        const Base::Vector3f& n = normals[index];
        if (!boost::math::isnan(p.x) && !boost::math::isnan(p.y) && !boost::math::isnan(p.z)) {
            PointNormal pn;
            pn.x = p.x;
            pn.y = p.y;
            pn.z = p.z;
            pn.normal_x = n.x;
            pn.normal_y = n.y;
            pn.normal_z = n.z;
            cloud_with_normals->push_back(pn);
        }
    }

    // Create search tree
    tree.reset(new search::KdTree<PointNormal>);
    tree->setInputCloud(cloud_with_normals);

    // Init objects
    GridProjection<PointNormal> grid;

    // Set parameters
    grid.setResolution(0.005);
    grid.setPaddingSize(3);
    grid.setNearestNeighborNum(100);
    grid.setMaxBinarySearchLevel(10);
    grid.setInputCloud(cloud_with_normals);
    grid.setSearchMethod(tree);

    // Reconstruct
    PolygonMesh mesh;
    grid.reconstruct(mesh);

    MeshConversion::convert(mesh, myMesh);
}

// ----------------------------------------------------------------------------

ImageTriangulation::ImageTriangulation(int width,
                                       int height,
                                       const Points::PointKernel& pts,
                                       Mesh::MeshObject& mesh)
    : width(width)
    , height(height)
    , myPoints(pts)
    , myMesh(mesh)
{}

void ImageTriangulation::perform()
{
    if (myPoints.size() != static_cast<std::size_t>(width * height)) {
        throw Base::RuntimeError("Number of points doesn't match with given width and height");
    }

    // construct dataset
    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_organized(new pcl::PointCloud<pcl::PointXYZ>());
    cloud_organized->width = width;
    cloud_organized->height = height;
    cloud_organized->points.resize(cloud_organized->width * cloud_organized->height);

    const std::vector<Base::Vector3f>& points = myPoints.getBasicPoints();

    int npoints = 0;
    for (size_t i = 0; i < cloud_organized->height; i++) {
        for (size_t j = 0; j < cloud_organized->width; j++) {
            const Base::Vector3f& p = points[npoints];
            cloud_organized->points[npoints].x = p.x;
            cloud_organized->points[npoints].y = p.y;
            cloud_organized->points[npoints].z = p.z;
            npoints++;
        }
    }

    OrganizedFastMesh<PointXYZ> ofm;

    // Set parameters
    ofm.setInputCloud(cloud_organized);
    // This parameter is not yet implemented (pcl 1.7)
    ofm.setMaxEdgeLength(1.5);
    ofm.setTrianglePixelSize(1);
    ofm.setTriangulationType(OrganizedFastMesh<PointXYZ>::TRIANGLE_ADAPTIVE_CUT);
    ofm.storeShadowedFaces(true);

    // Reconstruct
    PolygonMesh mesh;
    ofm.reconstruct(mesh);

    MeshConversion::convert(mesh, myMesh);

    // remove invalid points
    //
    MeshCore::MeshKernel& kernel = myMesh.getKernel();
    const MeshCore::MeshFacetArray& face = kernel.GetFacets();
    MeshCore::MeshAlgorithm meshAlg(kernel);
    meshAlg.SetPointFlag(MeshCore::MeshPoint::INVALID);
    std::vector<MeshCore::PointIndex> validPoints;
    validPoints.reserve(face.size() * 3);
    for (MeshCore::MeshFacetArray::_TConstIterator it = face.begin(); it != face.end(); ++it) {
        validPoints.push_back(it->_aulPoints[0]);
        validPoints.push_back(it->_aulPoints[1]);
        validPoints.push_back(it->_aulPoints[2]);
    }

    // remove duplicates
    std::sort(validPoints.begin(), validPoints.end());
    validPoints.erase(std::unique(validPoints.begin(), validPoints.end()), validPoints.end());
    meshAlg.ResetPointsFlag(validPoints, MeshCore::MeshPoint::INVALID);

    unsigned long countInvalid = meshAlg.CountPointFlag(MeshCore::MeshPoint::INVALID);
    if (countInvalid > 0) {
        std::vector<MeshCore::PointIndex> invalidPoints;
        invalidPoints.reserve(countInvalid);
        meshAlg.GetPointsFlag(invalidPoints, MeshCore::MeshPoint::INVALID);

        kernel.DeletePoints(invalidPoints);
    }
}

// ----------------------------------------------------------------------------

Reen::MarchingCubesRBF::MarchingCubesRBF(const Points::PointKernel& pts, Mesh::MeshObject& mesh)
    : myPoints(pts)
    , myMesh(mesh)
{}

void Reen::MarchingCubesRBF::perform(int ksearch)
{
    PointCloud<PointXYZ>::Ptr cloud(new PointCloud<PointXYZ>);
    PointCloud<PointNormal>::Ptr cloud_with_normals(new PointCloud<PointNormal>);
    search::KdTree<PointXYZ>::Ptr tree;
    search::KdTree<PointNormal>::Ptr tree2;

    cloud->reserve(myPoints.size());
    for (Points::PointKernel::const_iterator it = myPoints.begin(); it != myPoints.end(); ++it) {
        if (!boost::math::isnan(it->x) && !boost::math::isnan(it->y)
            && !boost::math::isnan(it->z)) {
            cloud->push_back(PointXYZ(it->x, it->y, it->z));
        }
    }

    // Create search tree
    tree.reset(new search::KdTree<PointXYZ>(false));
    tree->setInputCloud(cloud);

    // Normal estimation
    NormalEstimation<PointXYZ, Normal> n;
    PointCloud<Normal>::Ptr normals(new PointCloud<Normal>());
    n.setInputCloud(cloud);
    // n.setIndices (indices[B);
    n.setSearchMethod(tree);
    n.setKSearch(ksearch);
    n.compute(*normals);

    // Concatenate XYZ and normal information
    pcl::concatenateFields(*cloud, *normals, *cloud_with_normals);

    // Create search tree
    tree2.reset(new search::KdTree<PointNormal>);
    tree2->setInputCloud(cloud_with_normals);

    // Init objects
    pcl::MarchingCubesRBF<PointNormal> rbf;

    // Set parameters
    rbf.setIsoLevel(0);
    rbf.setGridResolution(60, 60, 60);
    rbf.setPercentageExtendGrid(0.1f);
    rbf.setOffSurfaceDisplacement(0.02f);

    rbf.setInputCloud(cloud_with_normals);
    rbf.setSearchMethod(tree2);

    // Reconstruct
    PolygonMesh mesh;
    rbf.reconstruct(mesh);

    MeshConversion::convert(mesh, myMesh);
}

void Reen::MarchingCubesRBF::perform(const std::vector<Base::Vector3f>& normals)
{
    if (myPoints.size() != normals.size()) {
        throw Base::RuntimeError("Number of points doesn't match with number of normals");
    }

    PointCloud<PointNormal>::Ptr cloud_with_normals(new PointCloud<PointNormal>);
    search::KdTree<PointNormal>::Ptr tree;

    cloud_with_normals->reserve(myPoints.size());
    std::size_t num_points = myPoints.size();
    const std::vector<Base::Vector3f>& points = myPoints.getBasicPoints();
    for (std::size_t index = 0; index < num_points; index++) {
        const Base::Vector3f& p = points[index];
        const Base::Vector3f& n = normals[index];
        if (!boost::math::isnan(p.x) && !boost::math::isnan(p.y) && !boost::math::isnan(p.z)) {
            PointNormal pn;
            pn.x = p.x;
            pn.y = p.y;
            pn.z = p.z;
            pn.normal_x = n.x;
            pn.normal_y = n.y;
            pn.normal_z = n.z;
            cloud_with_normals->push_back(pn);
        }
    }

    // Create search tree
    tree.reset(new search::KdTree<PointNormal>);
    tree->setInputCloud(cloud_with_normals);


    // Init objects
    pcl::MarchingCubesRBF<PointNormal> rbf;

    // Set parameters
    rbf.setIsoLevel(0);
    rbf.setGridResolution(60, 60, 60);
    rbf.setPercentageExtendGrid(0.1f);
    rbf.setOffSurfaceDisplacement(0.02f);

    rbf.setInputCloud(cloud_with_normals);
    rbf.setSearchMethod(tree);

    // Reconstruct
    PolygonMesh mesh;
    rbf.reconstruct(mesh);

    MeshConversion::convert(mesh, myMesh);
}

// ----------------------------------------------------------------------------

Reen::MarchingCubesHoppe::MarchingCubesHoppe(const Points::PointKernel& pts, Mesh::MeshObject& mesh)
    : myPoints(pts)
    , myMesh(mesh)
{}

void Reen::MarchingCubesHoppe::perform(int ksearch)
{
    PointCloud<PointXYZ>::Ptr cloud(new PointCloud<PointXYZ>);
    PointCloud<PointNormal>::Ptr cloud_with_normals(new PointCloud<PointNormal>);
    search::KdTree<PointXYZ>::Ptr tree;
    search::KdTree<PointNormal>::Ptr tree2;

    cloud->reserve(myPoints.size());
    for (Points::PointKernel::const_iterator it = myPoints.begin(); it != myPoints.end(); ++it) {
        if (!boost::math::isnan(it->x) && !boost::math::isnan(it->y)
            && !boost::math::isnan(it->z)) {
            cloud->push_back(PointXYZ(it->x, it->y, it->z));
        }
    }

    // Create search tree
    tree.reset(new search::KdTree<PointXYZ>(false));
    tree->setInputCloud(cloud);

    // Normal estimation
    NormalEstimation<PointXYZ, Normal> n;
    PointCloud<Normal>::Ptr normals(new PointCloud<Normal>());
    n.setInputCloud(cloud);
    // n.setIndices (indices[B);
    n.setSearchMethod(tree);
    n.setKSearch(ksearch);
    n.compute(*normals);

    // Concatenate XYZ and normal information
    pcl::concatenateFields(*cloud, *normals, *cloud_with_normals);

    // Create search tree
    tree2.reset(new search::KdTree<PointNormal>);
    tree2->setInputCloud(cloud_with_normals);

    // Init objects
    pcl::MarchingCubesHoppe<PointNormal> hoppe;

    // Set parameters
    hoppe.setIsoLevel(0);
    hoppe.setGridResolution(60, 60, 60);
    hoppe.setPercentageExtendGrid(0.1f);

    hoppe.setInputCloud(cloud_with_normals);
    hoppe.setSearchMethod(tree2);

    // Reconstruct
    PolygonMesh mesh;
    hoppe.reconstruct(mesh);

    MeshConversion::convert(mesh, myMesh);
}

void Reen::MarchingCubesHoppe::perform(const std::vector<Base::Vector3f>& normals)
{
    if (myPoints.size() != normals.size()) {
        throw Base::RuntimeError("Number of points doesn't match with number of normals");
    }

    PointCloud<PointNormal>::Ptr cloud_with_normals(new PointCloud<PointNormal>);
    search::KdTree<PointNormal>::Ptr tree;

    cloud_with_normals->reserve(myPoints.size());
    std::size_t num_points = myPoints.size();
    const std::vector<Base::Vector3f>& points = myPoints.getBasicPoints();
    for (std::size_t index = 0; index < num_points; index++) {
        const Base::Vector3f& p = points[index];
        const Base::Vector3f& n = normals[index];
        if (!boost::math::isnan(p.x) && !boost::math::isnan(p.y) && !boost::math::isnan(p.z)) {
            PointNormal pn;
            pn.x = p.x;
            pn.y = p.y;
            pn.z = p.z;
            pn.normal_x = n.x;
            pn.normal_y = n.y;
            pn.normal_z = n.z;
            cloud_with_normals->push_back(pn);
        }
    }

    // Create search tree
    tree.reset(new search::KdTree<PointNormal>);
    tree->setInputCloud(cloud_with_normals);


    // Init objects
    pcl::MarchingCubesHoppe<PointNormal> hoppe;

    // Set parameters
    hoppe.setIsoLevel(0);
    hoppe.setGridResolution(60, 60, 60);
    hoppe.setPercentageExtendGrid(0.1f);

    hoppe.setInputCloud(cloud_with_normals);
    hoppe.setSearchMethod(tree);

    // Reconstruct
    PolygonMesh mesh;
    hoppe.reconstruct(mesh);

    MeshConversion::convert(mesh, myMesh);
}

// ----------------------------------------------------------------------------

void MeshConversion::convert(const pcl::PolygonMesh& pclMesh, Mesh::MeshObject& meshObject)
{
    // number of points
    size_t nr_points = pclMesh.cloud.width * pclMesh.cloud.height;
    size_t point_size = pclMesh.cloud.data.size() / nr_points;
    // number of faces for header
    size_t nr_faces = pclMesh.polygons.size();

    MeshCore::MeshPointArray points;
    points.reserve(nr_points);
    MeshCore::MeshFacetArray facets;
    facets.reserve(nr_faces);

    // get vertices
    MeshCore::MeshPoint vertex;
    for (size_t i = 0; i < nr_points; ++i) {
        int xyz = 0;
        for (size_t d = 0; d < pclMesh.cloud.fields.size(); ++d) {
            int c = 0;
            // adding vertex
            if ((pclMesh.cloud.fields[d].datatype ==
#if PCL_VERSION_COMPARE(>, 1, 6, 0)
                 pcl::PCLPointField::FLOAT32)
                &&
#else
                 sensor_msgs::PointField::FLOAT32)
                &&
#endif
                (pclMesh.cloud.fields[d].name == "x" || pclMesh.cloud.fields[d].name == "y"
                 || pclMesh.cloud.fields[d].name == "z")) {
                float value;
                memcpy(&value,
                       &pclMesh.cloud.data[i * point_size + pclMesh.cloud.fields[d].offset
                                           + c * sizeof(float)],
                       sizeof(float));
                vertex[xyz] = value;
                if (++xyz == 3) {
                    points.push_back(vertex);
                    break;
                }
            }
        }
    }
    // get faces
    MeshCore::MeshFacet face;
    for (size_t i = 0; i < nr_faces; i++) {
        face._aulPoints[0] = pclMesh.polygons[i].vertices[0];
        face._aulPoints[1] = pclMesh.polygons[i].vertices[1];
        face._aulPoints[2] = pclMesh.polygons[i].vertices[2];
        facets.push_back(face);
    }

    MeshCore::MeshKernel kernel;
    kernel.Adopt(points, facets, true);
    meshObject.swap(kernel);
    meshObject.harmonizeNormals();
}

#endif  // HAVE_PCL_SURFACE
