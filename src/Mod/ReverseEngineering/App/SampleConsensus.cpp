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

#include "SampleConsensus.h"
#include <Mod/Points/App/Points.h>
#include <Base/Exception.h>
#include <boost/math/special_functions/fpclassify.hpp>

#if defined(HAVE_PCL_SAMPLE_CONSENSUS)
#include <pcl/point_types.h>
#include <pcl/sample_consensus/ransac.h>
#include <pcl/sample_consensus/sac_model_plane.h>

using namespace std;
using namespace Reen;
using pcl::PointXYZ;
using pcl::PointNormal;
using pcl::PointCloud;

SampleConsensus::SampleConsensus(const Points::PointKernel& pts)
  : myPoints(pts)
{
}

double SampleConsensus::perform(std::vector<float>& parameters)
{
    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud (new pcl::PointCloud<pcl::PointXYZ>);
    cloud->reserve(myPoints.size());
    for (Points::PointKernel::const_iterator it = myPoints.begin(); it != myPoints.end(); ++it) {
        if (!boost::math::isnan(it->x) && !boost::math::isnan(it->y) && !boost::math::isnan(it->z))
            cloud->push_back(pcl::PointXYZ(it->x, it->y, it->z));
    }

    cloud->width = int (cloud->points.size ());
    cloud->height = 1;
    cloud->is_dense = true;

    // created RandomSampleConsensus object and compute the appropriated model
    pcl::SampleConsensusModelPlane<pcl::PointXYZ>::Ptr
    model_p (new pcl::SampleConsensusModelPlane<pcl::PointXYZ> (cloud));

    pcl::RandomSampleConsensus<pcl::PointXYZ> ransac (model_p);
    ransac.setDistanceThreshold (.01);
    ransac.computeModel();
    //ransac.getInliers(inliers);
    //ransac.getModel (model);
    Eigen::VectorXf model_p_coefficients;
    ransac.getModelCoefficients (model_p_coefficients);
    for (int i=0; i<model_p_coefficients.size(); i++)
        parameters.push_back(model_p_coefficients[i]);

    return ransac.getProbability();
}

#endif // HAVE_PCL_SAMPLE_CONSENSUS

