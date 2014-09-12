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

#include "SurfaceTriangulation.h"
#include <Mod/Points/App/Points.h>
#include <Mod/Mesh/App/Mesh.h>
#include <Mod/Mesh/App/Core/Elements.h>
#include <Mod/Mesh/App/Core/MeshKernel.h>

// http://svn.pointclouds.org/pcl/tags/pcl-1.5.1/test/
#if defined(HAVE_PCL_SURFACE)
#include <pcl/pcl_config.h>
#include <pcl/point_types.h>
#include <pcl/features/normal_3d.h>
#include <pcl/surface/mls.h>
#include <pcl/point_traits.h>
#include <pcl/surface/gp3.h>
#include <pcl/surface/grid_projection.h>
//#include <pcl/surface/convex_hull.h>
//#include <pcl/surface/concave_hull.h>
#include <pcl/surface/organized_fast_mesh.h>
#include <pcl/surface/ear_clipping.h>
#include <pcl/common/common.h>
#include <boost/random.hpp>

#ifndef PCL_REVISION_VERSION
#define PCL_REVISION_VERSION 0
#endif

using namespace pcl;
using namespace pcl::io;
using namespace std;
using namespace Reen;

SurfaceTriangulation::SurfaceTriangulation(const Points::PointKernel& pts, Mesh::MeshObject& mesh)
  : myPoints(pts), myMesh(mesh)
{
}

void SurfaceTriangulation::perform()
{
    PointCloud<PointXYZ>::Ptr cloud (new PointCloud<PointXYZ>);
    PointCloud<PointNormal>::Ptr cloud_with_normals (new PointCloud<PointNormal>);
    search::KdTree<PointXYZ>::Ptr tree;
    search::KdTree<PointNormal>::Ptr tree2;
    
    for (Points::PointKernel::const_iterator it = myPoints.begin(); it != myPoints.end(); ++it) {
        cloud->push_back(PointXYZ(it->x, it->y, it->z));
    }

    // Create search tree
    tree.reset (new search::KdTree<PointXYZ> (false));
    tree->setInputCloud (cloud);

    // Normal estimation
    NormalEstimation<PointXYZ, Normal> n;
    PointCloud<Normal>::Ptr normals (new PointCloud<Normal> ());
    n.setInputCloud (cloud);
    //n.setIndices (indices[B);
    n.setSearchMethod (tree);
    n.setKSearch (20);
    n.compute (*normals);

    // Concatenate XYZ and normal information
    pcl::concatenateFields (*cloud, *normals, *cloud_with_normals);
      
    // Create search tree
    tree2.reset (new search::KdTree<PointNormal>);
    tree2->setInputCloud (cloud_with_normals);

    // Init objects
    GreedyProjectionTriangulation<PointNormal> gp3;

    // Set parameters
    gp3.setInputCloud (cloud_with_normals);
    gp3.setSearchMethod (tree2);
    gp3.setSearchRadius (2.025);
    gp3.setMu (2.5);
    gp3.setMaximumNearestNeighbors (100);
    gp3.setMaximumSurfaceAngle(M_PI/4); // 45 degrees
    gp3.setMinimumAngle(M_PI/18); // 10 degrees
    gp3.setMaximumAngle(2*M_PI/3); // 120 degrees
    gp3.setNormalConsistency(false);

    // Reconstruct
    PolygonMesh mesh;
    gp3.reconstruct (mesh);

    // number of points
    size_t nr_points  = mesh.cloud.width * mesh.cloud.height;
    size_t point_size = mesh.cloud.data.size () / nr_points;
    // number of faces for header
    size_t nr_faces = mesh.polygons.size ();

    MeshCore::MeshPointArray points;
    points.reserve(nr_points);
    MeshCore::MeshFacetArray facets;
    facets.reserve(nr_faces);

    // get vertices
    MeshCore::MeshPoint vertex;
    for (size_t i = 0; i < nr_points; ++i) {
        int xyz = 0;
        for (size_t d = 0; d < mesh.cloud.fields.size(); ++d) {
            int c = 0;
            // adding vertex
            if ((mesh.cloud.fields[d].datatype ==
#if PCL_VERSION_COMPARE(>,1,6,0)
                 pcl::PCLPointField::FLOAT32) &&
#else
                 sensor_msgs::PointField::FLOAT32) &&
#endif
                (mesh.cloud.fields[d].name == "x" ||
                 mesh.cloud.fields[d].name == "y" ||
                 mesh.cloud.fields[d].name == "z"))
            {
                float value;
                memcpy (&value, &mesh.cloud.data[i * point_size + mesh.cloud.fields[d].offset + c * sizeof (float)], sizeof (float));
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
        face._aulPoints[0] = mesh.polygons[i].vertices[0];
        face._aulPoints[1] = mesh.polygons[i].vertices[1];
        face._aulPoints[2] = mesh.polygons[i].vertices[2];
        facets.push_back(face);
    }

    MeshCore::MeshKernel kernel;
    kernel.Adopt(points, facets, true);
    myMesh.swap(kernel);
    myMesh.harmonizeNormals();

    // Additional vertex information
    //std::vector<int> parts = gp3.getPartIDs();
    //std::vector<int> states = gp3.getPointStates();
}

#endif // HAVE_PCL_SURFACE

