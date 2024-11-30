/***************************************************************************
 *   Copyright (c) 2022 Matteo Grellier <matteogrellier@gmail.com>         *
 *                                                                         *
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
#include <BRepAdaptor_Curve.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <Standard_Version.hxx>
#include <TopoDS.hxx>
#include <gp_Pnt.hxx>
#endif

#include "Mod/Surface/App/Blending/BlendCurve.h"
#include "Mod/Surface/App/Blending/BlendPoint.h"
#include <Base/Tools.h>

#include "FeatureBlendCurve.h"


using namespace Surface;

const App::PropertyFloatConstraint::Constraints ParameterConstraint = {0.0, 1.0, 0.05};
const App::PropertyIntegerConstraint::Constraints ContinuityConstraint = {0, 25, 1};
const App::PropertyFloatConstraint::Constraints SizeConstraint = {-100.0, 100.0, 0.1};

PROPERTY_SOURCE(Surface::FeatureBlendCurve, Part::Spline)

FeatureBlendCurve::FeatureBlendCurve()
{
    ADD_PROPERTY_TYPE(StartEdge,
                      (nullptr),
                      "FirstEdge",
                      App::Prop_None,
                      "Edge support of the start point");
    ADD_PROPERTY_TYPE(StartContinuity,
                      (2),
                      "FirstEdge",
                      App::Prop_None,
                      "Geometric continuity at start point");
    StartContinuity.setConstraints(&ContinuityConstraint);
    ADD_PROPERTY_TYPE(StartParameter,
                      (0.0f),
                      "FirstEdge",
                      App::Prop_None,
                      "Parameter of start point along edge");
    StartParameter.setConstraints(&ParameterConstraint);
    ADD_PROPERTY_TYPE(StartSize,
                      (1.0f),
                      "FirstEdge",
                      App::Prop_None,
                      "Size of derivatives at start point");
    StartSize.setConstraints(&SizeConstraint);

    ADD_PROPERTY_TYPE(EndEdge,
                      (nullptr),
                      "SecondEdge",
                      App::Prop_None,
                      "Edge support of the end point");
    ADD_PROPERTY_TYPE(EndContinuity,
                      (2),
                      "SecondEdge",
                      App::Prop_None,
                      "Geometric continuity at end point");
    EndContinuity.setConstraints(&ContinuityConstraint);
    ADD_PROPERTY_TYPE(EndParameter,
                      (0.0f),
                      "SecondEdge",
                      App::Prop_None,
                      "Parameter of end point along edge");
    EndParameter.setConstraints(&ParameterConstraint);
    ADD_PROPERTY_TYPE(EndSize,
                      (1.0f),
                      "SecondEdge",
                      App::Prop_None,
                      "Size of derivatives at end point");
    EndSize.setConstraints(&SizeConstraint);

    Handle(Geom_BezierCurve) maxDegreeCurve;
    maxDegree = maxDegreeCurve->MaxDegree();
}

short FeatureBlendCurve::mustExecute() const
{
    if (StartEdge.isTouched()) {
        return 1;
    }
    if (StartParameter.isTouched()) {
        return 1;
    }
    if (StartContinuity.isTouched()) {
        return 1;
    }
    if (StartSize.isTouched()) {
        return 1;
    }
    if (EndEdge.isTouched()) {
        return 1;
    }
    if (EndParameter.isTouched()) {
        return 1;
    }
    if (EndContinuity.isTouched()) {
        return 1;
    }
    if (EndSize.isTouched()) {
        return 1;
    }
    return 0;
}

BlendPoint FeatureBlendCurve::GetBlendPoint(App::PropertyLinkSub& link,
                                            App::PropertyFloatConstraint& param,
                                            App::PropertyIntegerConstraint& continuity)
{
    auto linked = link.getValue();

    TopoDS_Shape axEdge;
    if (link.getSubValues().size() > 0 && link.getSubValues()[0].length() > 0) {
        axEdge =
            Feature::getTopoShape(linked, link.getSubValues()[0].c_str(), true /*need element*/)
                .getShape();
    }
    else {
        axEdge = Feature::getShape(linked);
    }

    if (axEdge.IsNull()) {
        throw Base::ValueError("DirLink shape is null");
    }
    if (axEdge.ShapeType() != TopAbs_EDGE) {
        throw Base::TypeError("DirLink shape is not an edge");
    }
    const TopoDS_Edge& e = TopoDS::Edge(axEdge);
    BRepAdaptor_Curve adapt(e);
    double fp = adapt.FirstParameter();
    double lp = adapt.LastParameter();

    double RealPar = RelativeToRealParameters(param.getValue(), fp, lp);

    std::vector<Base::Vector3d> constraints;
    gp_Pnt Pt;

    adapt.D0(RealPar, Pt);
    Base::Vector3d bv(Pt.X(), Pt.Y(), Pt.Z());
    constraints.emplace_back(bv);

    for (int i = 1; i <= continuity.getValue(); i++) {
        gp_Vec v1 = adapt.DN(RealPar, i);
        Base::Vector3d bbv1(v1.X(), v1.Y(), v1.Z());
        constraints.emplace_back(bbv1);
    }

    BlendPoint bp(constraints);

    return bp;
}

App::DocumentObjectExecReturn* FeatureBlendCurve::execute()
{
    BlendPoint bp1 = GetBlendPoint(StartEdge, StartParameter, StartContinuity);
    BlendPoint bp2 = GetBlendPoint(EndEdge, EndParameter, EndContinuity);

    std::vector<BlendPoint> blendPointsList;

    blendPointsList.emplace_back(bp1);
    blendPointsList.emplace_back(bp2);

    BlendCurve curve(blendPointsList);
    curve.setSize(0, StartSize.getValue(), true);
    curve.setSize(1, EndSize.getValue(), true);

    Handle(Geom_BezierCurve) bc(curve.compute());
    BRepBuilderAPI_MakeEdge mkEdge(bc);

    Shape.setValue(mkEdge.Edge());

    return StdReturn;
}

double FeatureBlendCurve::RelativeToRealParameters(double relativeValue, double fp, double lp)
{
    return fp + relativeValue * (lp - fp);
}


void FeatureBlendCurve::onChanged(const App::Property* prop)
{
    if (prop == &StartContinuity) {
        long maxValue = maxDegree - 2 - EndContinuity.getValue();
        if (StartContinuity.getValue() > maxValue) {
            StartContinuity.setValue(maxValue);
        }
    }
    else if (prop == &EndContinuity) {
        long maxValue = maxDegree - 2 - StartContinuity.getValue();
        if (EndContinuity.getValue() > maxValue) {
            EndContinuity.setValue(maxValue);
        }
    }
    if (prop == &StartContinuity || prop == &StartParameter || prop == &StartSize
        || prop == &EndContinuity || prop == &EndParameter || prop == &EndSize) {
        if (!isRestoring()) {
            App::DocumentObjectExecReturn* ret = recompute();
            delete ret;
        }
    }
    Part::Spline::onChanged(prop);
}
