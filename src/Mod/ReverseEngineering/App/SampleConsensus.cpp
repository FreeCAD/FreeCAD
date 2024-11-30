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

#include <Base/Exception.h>
#include <Mod/Points/App/Points.h>

#include "SampleConsensus.h"


#if defined(HAVE_PCL_SAMPLE_CONSENSUS)
#include <pcl/features/normal_3d.h>
#include <pcl/point_types.h>
#include <pcl/sample_consensus/ransac.h>
#include <pcl/sample_consensus/sac_model_cone.h>
#include <pcl/sample_consensus/sac_model_cylinder.h>
#include <pcl/sample_consensus/sac_model_plane.h>
#include <pcl/sample_consensus/sac_model_sphere.h>

using namespace std;
using namespace Reen;
using pcl::PointCloud;
using pcl::PointNormal;
using pcl::PointXYZ;

SampleConsensus::SampleConsensus(SacModel sac,
                                 const Points::PointKernel& pts,
                                 const std::vector<Base::Vector3d>& nor)
    : mySac(sac)
    , myPoints(pts)
    , myNormals(nor)
{}

double SampleConsensus::perform(std::vector<float>& parameters, std::vector<int>& model)
{
    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZ>);
    cloud->reserve(myPoints.size());
    for (Points::PointKernel::const_iterator it = myPoints.begin(); it != myPoints.end(); ++it) {
        if (!boost::math::isnan(it->x) && !boost::math::isnan(it->y)
            && !boost::math::isnan(it->z)) {
            cloud->push_back(pcl::PointXYZ(it->x, it->y, it->z));
        }
    }

    cloud->width = int(cloud->points.size());
    cloud->height = 1;
    cloud->is_dense = true;

    pcl::PointCloud<pcl::Normal>::Ptr normals(new pcl::PointCloud<pcl::Normal>());
    if (mySac == SACMODEL_CONE || mySac == SACMODEL_CYLINDER) {
#if 0
        // Create search tree
        pcl::search::KdTree<pcl::PointXYZ>::Ptr tree;
        tree.reset (new pcl::search::KdTree<PointXYZ> (false));
        tree->setInputCloud (cloud);

        // Normal estimation
        int ksearch = 10;
        pcl::NormalEstimation<pcl::PointXYZ, pcl::Normal> n;
        n.setInputCloud (cloud);
        n.setSearchMethod (tree);
        n.setKSearch (ksearch);
        n.compute (*normals);
#else
        normals->reserve(myNormals.size());
        for (std::vector<Base::Vector3d>::const_iterator it = myNormals.begin();
             it != myNormals.end();
             ++it) {
            if (!boost::math::isnan(it->x) && !boost::math::isnan(it->y)
                && !boost::math::isnan(it->z)) {
                normals->push_back(pcl::Normal(it->x, it->y, it->z));
            }
        }
#endif
    }

    // created RandomSampleConsensus object and compute the appropriated model
    pcl::SampleConsensusModel<pcl::PointXYZ>::Ptr model_p;
    switch (mySac) {
        case SACMODEL_PLANE: {
            model_p.reset(new pcl::SampleConsensusModelPlane<pcl::PointXYZ>(cloud));
            break;
        }
        case SACMODEL_SPHERE: {
            model_p.reset(new pcl::SampleConsensusModelSphere<pcl::PointXYZ>(cloud));
            break;
        }
        case SACMODEL_CONE: {
            pcl::SampleConsensusModelCone<pcl::PointXYZ, pcl::Normal>::Ptr model_c(
                new pcl::SampleConsensusModelCone<pcl::PointXYZ, pcl::Normal>(cloud));
            model_c->setInputNormals(normals);
            model_p = model_c;
            break;
        }
        case SACMODEL_CYLINDER: {
            pcl::SampleConsensusModelCylinder<pcl::PointXYZ, pcl::Normal>::Ptr model_c(
                new pcl::SampleConsensusModelCylinder<pcl::PointXYZ, pcl::Normal>(cloud));
            model_c->setInputNormals(normals);
            model_p = model_c;
            break;
        }
        default:
            throw Base::RuntimeError("Unsupported SAC model");
    }

    pcl::RandomSampleConsensus<pcl::PointXYZ> ransac(model_p);
    ransac.setDistanceThreshold(.01);
    ransac.computeModel();
    ransac.getInliers(model);
    // ransac.getModel (model);
    Eigen::VectorXf model_p_coefficients;
    ransac.getModelCoefficients(model_p_coefficients);
    for (int i = 0; i < model_p_coefficients.size(); i++) {
        parameters.push_back(model_p_coefficients[i]);
    }

    return ransac.getProbability();
}

#endif  // HAVE_PCL_SAMPLE_CONSENSUS
