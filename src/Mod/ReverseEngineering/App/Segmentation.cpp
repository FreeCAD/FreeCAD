/***************************************************************************
 *   Copyright (c) 2016 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <Mod/Points/App/Points.h>

#include "Segmentation.h"


#if defined(HAVE_PCL_FILTERS)
#include <pcl/features/normal_3d.h>
#include <pcl/filters/extract_indices.h>
#include <pcl/filters/passthrough.h>
#endif

#if defined(HAVE_PCL_SAMPLE_CONSENSUS)
#include <pcl/sample_consensus/method_types.h>
#include <pcl/sample_consensus/model_types.h>
#endif

#if defined(HAVE_PCL_SEGMENTATION)
#include <pcl/ModelCoefficients.h>
#include <pcl/io/pcd_io.h>
#include <pcl/point_types.h>
#include <pcl/segmentation/sac_segmentation.h>
#endif

using namespace std;
using namespace Reen;

#if defined(HAVE_PCL_FILTERS)
using pcl::PointCloud;
using pcl::PointNormal;
using pcl::PointXYZ;
#endif

#if defined(HAVE_PCL_SEGMENTATION)
Segmentation::Segmentation(const Points::PointKernel& pts, std::list<std::vector<int>>& clusters)
    : myPoints(pts)
    , myClusters(clusters)
{}

void Segmentation::perform(int ksearch)
{
    // All the objects needed
    pcl::PassThrough<PointXYZ> pass;
    pcl::NormalEstimation<PointXYZ, pcl::Normal> ne;
    pcl::SACSegmentationFromNormals<PointXYZ, pcl::Normal> seg;
    pcl::ExtractIndices<PointXYZ> extract;
    pcl::ExtractIndices<pcl::Normal> extract_normals;
    pcl::search::KdTree<PointXYZ>::Ptr tree(new pcl::search::KdTree<PointXYZ>());

    // Datasets
    pcl::PointCloud<PointXYZ>::Ptr cloud(new pcl::PointCloud<PointXYZ>);
    pcl::PointCloud<PointXYZ>::Ptr cloud_filtered(new pcl::PointCloud<PointXYZ>);
    pcl::PointCloud<pcl::Normal>::Ptr cloud_normals(new pcl::PointCloud<pcl::Normal>);
    pcl::PointCloud<PointXYZ>::Ptr cloud_filtered2(new pcl::PointCloud<PointXYZ>);
    pcl::PointCloud<pcl::Normal>::Ptr cloud_normals2(new pcl::PointCloud<pcl::Normal>);
    pcl::ModelCoefficients::Ptr coefficients_plane(new pcl::ModelCoefficients),
        coefficients_cylinder(new pcl::ModelCoefficients);
    pcl::PointIndices::Ptr inliers_plane(new pcl::PointIndices),
        inliers_cylinder(new pcl::PointIndices);

    // Copy the points
    cloud->reserve(myPoints.size());
    for (Points::PointKernel::const_iterator it = myPoints.begin(); it != myPoints.end(); ++it) {
        cloud->push_back(pcl::PointXYZ(it->x, it->y, it->z));
    }

    cloud->width = int(cloud->points.size());
    cloud->height = 1;

    // Build a passthrough filter to remove spurious NaNs
    pass.setInputCloud(cloud);
    pass.setFilterFieldName("z");
    pass.setFilterLimits(0, 1.5);
    pass.filter(*cloud_filtered);

    // Estimate point normals
    ne.setSearchMethod(tree);
    ne.setInputCloud(cloud_filtered);
    ne.setKSearch(ksearch);
    ne.compute(*cloud_normals);

    // Create the segmentation object for the planar model and set all the parameters
    seg.setOptimizeCoefficients(true);
    seg.setModelType(pcl::SACMODEL_NORMAL_PLANE);
    seg.setNormalDistanceWeight(0.1);
    seg.setMethodType(pcl::SAC_RANSAC);
    seg.setMaxIterations(100);
    seg.setDistanceThreshold(0.03);
    seg.setInputCloud(cloud_filtered);
    seg.setInputNormals(cloud_normals);

    // Obtain the plane inliers and coefficients
    seg.segment(*inliers_plane, *coefficients_plane);
    myClusters.push_back(inliers_plane->indices);

    // Extract the planar inliers from the input cloud
    extract.setInputCloud(cloud_filtered);
    extract.setIndices(inliers_plane);
    extract.setNegative(false);

    // Write the planar inliers to disk
    pcl::PointCloud<PointXYZ>::Ptr cloud_plane(new pcl::PointCloud<PointXYZ>());
    extract.filter(*cloud_plane);

    // Remove the planar inliers, extract the rest
    extract.setNegative(true);
    extract.filter(*cloud_filtered2);
    extract_normals.setNegative(true);
    extract_normals.setInputCloud(cloud_normals);
    extract_normals.setIndices(inliers_plane);
    extract_normals.filter(*cloud_normals2);

    // Create the segmentation object for cylinder segmentation and set all the parameters
    seg.setOptimizeCoefficients(true);
    seg.setModelType(pcl::SACMODEL_CYLINDER);
    seg.setNormalDistanceWeight(0.1);
    seg.setMethodType(pcl::SAC_RANSAC);
    seg.setMaxIterations(10000);
    seg.setDistanceThreshold(0.05);
    seg.setRadiusLimits(0, 0.1);
    seg.setInputCloud(cloud_filtered2);
    seg.setInputNormals(cloud_normals2);

    // Obtain the cylinder inliers and coefficients
    seg.segment(*inliers_cylinder, *coefficients_cylinder);
    myClusters.push_back(inliers_cylinder->indices);

    // Write the cylinder inliers to disk
    extract.setInputCloud(cloud_filtered2);
    extract.setIndices(inliers_cylinder);
    extract.setNegative(false);
    pcl::PointCloud<PointXYZ>::Ptr cloud_cylinder(new pcl::PointCloud<PointXYZ>());
    extract.filter(*cloud_cylinder);
}

#endif  // HAVE_PCL_SEGMENTATION

// ----------------------------------------------------------------------------

#if defined(HAVE_PCL_FILTERS)
NormalEstimation::NormalEstimation(const Points::PointKernel& pts)
    : myPoints(pts)
    , kSearch(0)
    , searchRadius(0)
{}

void NormalEstimation::perform(std::vector<Base::Vector3d>& normals)
{
    // Copy the points
    pcl::PointCloud<PointXYZ>::Ptr cloud(new pcl::PointCloud<PointXYZ>);
    cloud->reserve(myPoints.size());
    for (Points::PointKernel::const_iterator it = myPoints.begin(); it != myPoints.end(); ++it) {
        cloud->push_back(pcl::PointXYZ(it->x, it->y, it->z));
    }

    cloud->width = int(cloud->points.size());
    cloud->height = 1;

#if 0
    // Build a passthrough filter to remove spurious NaNs
    pcl::PointCloud<PointXYZ>::Ptr cloud_filtered (new pcl::PointCloud<PointXYZ>);
    pcl::PassThrough<PointXYZ> pass;
    pass.setInputCloud (cloud);
    pass.setFilterFieldName ("z");
    pass.setFilterLimits (0, 1.5);
    pass.filter (*cloud_filtered);
#endif

    // Estimate point normals
    pcl::PointCloud<pcl::Normal>::Ptr cloud_normals(new pcl::PointCloud<pcl::Normal>);
    pcl::search::KdTree<PointXYZ>::Ptr tree(new pcl::search::KdTree<PointXYZ>());
    pcl::NormalEstimation<PointXYZ, pcl::Normal> ne;
    ne.setSearchMethod(tree);
    // ne.setInputCloud (cloud_filtered);
    ne.setInputCloud(cloud);
    if (kSearch > 0) {
        ne.setKSearch(kSearch);
    }
    if (searchRadius > 0) {
        ne.setRadiusSearch(searchRadius);
    }
    ne.compute(*cloud_normals);

    normals.reserve(cloud_normals->size());
    for (pcl::PointCloud<pcl::Normal>::const_iterator it = cloud_normals->begin();
         it != cloud_normals->end();
         ++it) {
        normals.push_back(Base::Vector3d(it->normal_x, it->normal_y, it->normal_z));
    }
}

#endif  // HAVE_PCL_FILTERS
