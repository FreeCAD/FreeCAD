/***************************************************************************
 *   Copyright (c) 2015 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
#if defined(HAVE_PCL_OPENNURBS)
#ifndef _PreComp_
#include <map>

#include <Geom_BSplineSurface.hxx>
#include <TColStd_Array1OfInteger.hxx>
#include <TColStd_Array1OfReal.hxx>
#include <TColStd_Array2OfReal.hxx>
#include <TColgp_Array2OfPnt.hxx>
#endif

#include <Mod/Points/App/PointsPy.h>

#include "BSplineFitting.h"

#include <pcl/pcl_config.h>
#if PCL_VERSION_COMPARE(>=, 1, 7, 0)
#include <pcl/io/pcd_io.h>
#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <pcl/surface/on_nurbs/fitting_curve_2d_asdm.h>
#include <pcl/surface/on_nurbs/fitting_surface_tdm.h>
#endif

using namespace Reen;


BSplineFitting::BSplineFitting(const std::vector<Base::Vector3f>& pts)
    : myPoints(pts)
    , myIterations(10)
    , myOrder(3)
    , myRefinement(4)
    , myInteriorSmoothness(0.2)
    , myInteriorWeight(1.0)
    , myBoundarySmoothness(0.2)
    , myBoundaryWeight(0.0)
{}

void BSplineFitting::setIterations(unsigned value)
{
    myIterations = value;
}

void BSplineFitting::setOrder(unsigned value)
{
    myOrder = value;
}

void BSplineFitting::setRefinement(unsigned value)
{
    myRefinement = value;
}

void BSplineFitting::setInteriorSmoothness(double value)
{
    myInteriorSmoothness = value;
}

void BSplineFitting::setInteriorWeight(double value)
{
    myInteriorWeight = value;
}

void BSplineFitting::setBoundarySmoothness(double value)
{
    myBoundarySmoothness = value;
}

void BSplineFitting::setBoundaryWeight(double value)
{
    myBoundaryWeight = value;
}

Handle(Geom_BSplineSurface) BSplineFitting::perform()
{
#if PCL_VERSION_COMPARE(>=, 1, 7, 0)
    pcl::on_nurbs::NurbsDataSurface data;
    for (std::vector<Base::Vector3f>::const_iterator it = myPoints.begin(); it != myPoints.end();
         ++it) {
        if (!pcl_isnan(it->x) && !pcl_isnan(it->y) && !pcl_isnan(it->z)) {
            data.interior.push_back(Eigen::Vector3d(it->x, it->y, it->z));
        }
    }


    // fit B-spline surface
    //

    pcl::on_nurbs::FittingSurface::Parameter params;
    params.interior_smoothness = myInteriorSmoothness;
    params.interior_weight = myInteriorWeight;
    params.boundary_smoothness = myBoundarySmoothness;
    params.boundary_weight = myBoundaryWeight;

    // initialize
    ON_NurbsSurface nurbs = pcl::on_nurbs::FittingSurface::initNurbsPCABoundingBox(myOrder, &data);
    pcl::on_nurbs::FittingSurface fit(&data, nurbs);
    //  fit.setQuiet (false); // enable/disable debug output

    // surface refinement
    for (unsigned i = 0; i < myRefinement; i++) {
        fit.refine(0);
        fit.refine(1);
        fit.assemble(params);
        fit.solve();
    }

    // surface fitting with final refinement level
    for (unsigned i = 0; i < myIterations; i++) {
        fit.assemble(params);
        fit.solve();
    }

    // fit B-spline curve
#if 0
    // parameters
    pcl::on_nurbs::FittingCurve2dAPDM::FitParameter curve_params;
    curve_params.addCPsAccuracy = 5e-2;
    curve_params.addCPsIteration = 3;
    curve_params.maxCPs = 200;
    curve_params.accuracy = 1e-3;
    curve_params.iterations = 100;

    curve_params.param.closest_point_resolution = 0;
    curve_params.param.closest_point_weight = 1.0;
    curve_params.param.closest_point_sigma2 = 0.1;
    curve_params.param.interior_sigma2 = 0.00001;
    curve_params.param.smooth_concavity = 1.0;
    curve_params.param.smoothness = 1.0;

    // initialisation (circular)
    pcl::on_nurbs::NurbsDataCurve2d curve_data;
    curve_data.interior = data.interior_param;
    curve_data.interior_weight_function.push_back(true);
    ON_NurbsCurve curve_nurbs = pcl::on_nurbs::FittingCurve2dAPDM::initNurbsCurve2D(order, curve_data.interior);

    // curve fitting
    pcl::on_nurbs::FittingCurve2dASDM curve_fit (&curve_data, curve_nurbs);
    // curve_fit.setQuiet (false); // enable/disable debug output
    curve_fit.fitting (curve_params);
#endif

    // u parameters
    int numUKnots = fit.m_nurbs.KnotCount(0);
    int numUPoles = fit.m_nurbs.CVCount(0);
    int uDegree = fit.m_nurbs.Degree(0);
    bool uPeriodic = fit.m_nurbs.IsPeriodic(0) ? true : false;
    std::map<Standard_Real, int> uKnots;

    // v parameters
    int numVKnots = fit.m_nurbs.KnotCount(1);
    int numVPoles = fit.m_nurbs.CVCount(1);
    int vDegree = fit.m_nurbs.Degree(1);
    bool vPeriodic = fit.m_nurbs.IsPeriodic(1) ? true : false;
    std::map<Standard_Real, int> vKnots;

    TColgp_Array2OfPnt poles(1, numUPoles, 1, numVPoles);
    TColStd_Array2OfReal weights(1, numUPoles, 1, numVPoles);

    for (int i = 0; i < numUPoles; i++) {
        for (int j = 0; j < numVPoles; j++) {
            ON_3dPoint cv;
            fit.m_nurbs.GetCV(i, j, cv);
            poles.SetValue(i + 1, j + 1, gp_Pnt(cv.x, cv.y, cv.z));

            Standard_Real weight = fit.m_nurbs.Weight(i, j);
            weights.SetValue(i + 1, j + 1, weight);
        }
    }

    uKnots[fit.m_nurbs.SuperfluousKnot(0, 0)] = 1;
    uKnots[fit.m_nurbs.SuperfluousKnot(0, 1)] = 1;
    for (int i = 0; i < numUKnots; i++) {
        Standard_Real value = fit.m_nurbs.Knot(0, i);
        std::map<Standard_Real, int>::iterator it = uKnots.find(value);
        if (it == uKnots.end()) {
            uKnots[value] = 1;
        }
        else {
            it->second++;
        }
    }

    vKnots[fit.m_nurbs.SuperfluousKnot(1, 0)] = 1;
    vKnots[fit.m_nurbs.SuperfluousKnot(1, 1)] = 1;
    for (int i = 0; i < numVKnots; i++) {
        Standard_Real value = fit.m_nurbs.Knot(1, i);
        std::map<Standard_Real, int>::iterator it = vKnots.find(value);
        if (it == vKnots.end()) {
            vKnots[value] = 1;
        }
        else {
            it->second++;
        }
    }

    TColStd_Array1OfReal uKnotArray(1, uKnots.size());
    TColStd_Array1OfInteger uMultArray(1, uKnots.size());
    int index = 1;
    for (std::map<Standard_Real, int>::iterator it = uKnots.begin(); it != uKnots.end();
         ++it, index++) {
        uKnotArray.SetValue(index, it->first);
        uMultArray.SetValue(index, it->second);
    }

    TColStd_Array1OfReal vKnotArray(1, vKnots.size());
    TColStd_Array1OfInteger vMultArray(1, vKnots.size());
    index = 1;
    for (std::map<Standard_Real, int>::iterator it = vKnots.begin(); it != vKnots.end();
         ++it, index++) {
        vKnotArray.SetValue(index, it->first);
        vMultArray.SetValue(index, it->second);
    }

    Handle(Geom_BSplineSurface) spline = new Geom_BSplineSurface(poles,
                                                                 weights,
                                                                 uKnotArray,
                                                                 vKnotArray,
                                                                 uMultArray,
                                                                 vMultArray,
                                                                 uDegree,
                                                                 vDegree,
                                                                 uPeriodic,
                                                                 vPeriodic);
    return spline;
#else
    return Handle(Geom_BSplineSurface)();
#endif
}
#endif  // HAVE_PCL_OPENNURBS
