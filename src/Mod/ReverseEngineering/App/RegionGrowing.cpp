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
#ifndef _PreComp_
#include <boost/math/special_functions/fpclassify.hpp>
#endif

#include <Mod/Points/App/Points.h>

#include "RegionGrowing.h"


#if defined(HAVE_PCL_FILTERS)
#include <pcl/filters/passthrough.h>
#include <pcl/point_types.h>
#endif
#if defined(HAVE_PCL_SEGMENTATION)
#include <pcl/features/normal_3d.h>
#include <pcl/filters/extract_indices.h>
#include <pcl/search/kdtree.h>
#include <pcl/search/search.h>
#include <pcl/segmentation/region_growing.h>

using namespace std;
using namespace Reen;
using pcl::PointCloud;
using pcl::PointNormal;
using pcl::PointXYZ;

RegionGrowing::RegionGrowing(const Points::PointKernel& pts, std::list<std::vector<int>>& clusters)
    : myPoints(pts)
    , myClusters(clusters)
{}

void RegionGrowing::perform(int ksearch)
{
    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZ>);
    cloud->reserve(myPoints.size());
    for (Points::PointKernel::const_iterator it = myPoints.begin(); it != myPoints.end(); ++it) {
        if (!boost::math::isnan(it->x) && !boost::math::isnan(it->y)
            && !boost::math::isnan(it->z)) {
            cloud->push_back(pcl::PointXYZ(it->x, it->y, it->z));
        }
    }

    // normal estimation
    pcl::search::Search<pcl::PointXYZ>::Ptr tree(new pcl::search::KdTree<pcl::PointXYZ>);
    pcl::PointCloud<pcl::Normal>::Ptr normals(new pcl::PointCloud<pcl::Normal>);
    pcl::NormalEstimation<pcl::PointXYZ, pcl::Normal> normal_estimator;
    normal_estimator.setSearchMethod(tree);
    normal_estimator.setInputCloud(cloud);
    normal_estimator.setKSearch(ksearch);
    normal_estimator.compute(*normals);

    // pass through
    pcl::IndicesPtr indices(new std::vector<int>);
    pcl::PassThrough<pcl::PointXYZ> pass;
    pass.setInputCloud(cloud);
    pass.setFilterFieldName("z");
    pass.setFilterLimits(0.0, 1.0);
    pass.filter(*indices);

    pcl::RegionGrowing<pcl::PointXYZ, pcl::Normal> reg;
    reg.setMinClusterSize(50);
    reg.setMaxClusterSize(1000000);
    reg.setSearchMethod(tree);
    reg.setNumberOfNeighbours(30);
    reg.setInputCloud(cloud);
    // reg.setIndices (indices);
    reg.setInputNormals(normals);
    reg.setSmoothnessThreshold(3.0 / 180.0 * M_PI);
    reg.setCurvatureThreshold(1.0);

    std::vector<pcl::PointIndices> clusters;
    reg.extract(clusters);

    for (std::vector<pcl::PointIndices>::iterator it = clusters.begin(); it != clusters.end();
         ++it) {
        myClusters.push_back(std::vector<int>());
        myClusters.back().swap(it->indices);
    }
}

void RegionGrowing::perform(const std::vector<Base::Vector3f>& myNormals)
{
    if (myPoints.size() != myNormals.size()) {
        throw Base::RuntimeError("Number of points doesn't match with number of normals");
    }

    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZ>);
    cloud->reserve(myPoints.size());
    pcl::PointCloud<pcl::Normal>::Ptr normals(new pcl::PointCloud<pcl::Normal>);
    normals->reserve(myNormals.size());

    std::size_t num_points = myPoints.size();
    const std::vector<Base::Vector3f>& points = myPoints.getBasicPoints();
    for (std::size_t index = 0; index < num_points; index++) {
        const Base::Vector3f& p = points[index];
        const Base::Vector3f& n = myNormals[index];
        if (!boost::math::isnan(p.x) && !boost::math::isnan(p.y) && !boost::math::isnan(p.z)) {
            cloud->push_back(pcl::PointXYZ(p.x, p.y, p.z));
            normals->push_back(pcl::Normal(n.x, n.y, n.z));
        }
    }

    pcl::search::Search<pcl::PointXYZ>::Ptr tree(new pcl::search::KdTree<pcl::PointXYZ>);
    tree->setInputCloud(cloud);

    // pass through
    pcl::IndicesPtr indices(new std::vector<int>);
    pcl::PassThrough<pcl::PointXYZ> pass;
    pass.setInputCloud(cloud);
    pass.setFilterFieldName("z");
    pass.setFilterLimits(0.0, 1.0);
    pass.filter(*indices);

    pcl::RegionGrowing<pcl::PointXYZ, pcl::Normal> reg;
    reg.setMinClusterSize(50);
    reg.setMaxClusterSize(1000000);
    reg.setSearchMethod(tree);
    reg.setNumberOfNeighbours(30);
    reg.setInputCloud(cloud);
    // reg.setIndices (indices);
    reg.setInputNormals(normals);
    reg.setSmoothnessThreshold(3.0 / 180.0 * M_PI);
    reg.setCurvatureThreshold(1.0);

    std::vector<pcl::PointIndices> clusters;
    reg.extract(clusters);

    for (std::vector<pcl::PointIndices>::iterator it = clusters.begin(); it != clusters.end();
         ++it) {
        myClusters.push_back(std::vector<int>());
        myClusters.back().swap(it->indices);
    }
}

#endif  // HAVE_PCL_SEGMENTATION
