/***************************************************************************
 *   Copyright (c) 2023 David Friedli <david[at]friedli-be.ch>             *
 *   Copyright (c) 2013 Thomas Anderson <blobfish[at]gmx.com>              *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#include "PreCompiled.h"

#ifndef _PreComp_
#include <sstream>
#include <QApplication>
#include <Inventor/draggers/SoTranslate2Dragger.h>
#include <Inventor/nodes/SoAnnotation.h>
#include <Inventor/nodes/SoBaseColor.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoFontStyle.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <Inventor/nodes/SoPickStyle.h>
#include <Inventor/nodes/SoText2.h>
#include <Inventor/nodes/SoTranslation.h>
#include <Inventor/engines/SoCalculator.h>
#include <Inventor/engines/SoComposeVec3f.h>
#include <Inventor/engines/SoConcatenate.h>
#include <Inventor/engines/SoComposeMatrix.h>
#include <Inventor/engines/SoComposeRotation.h>
#include <Inventor/engines/SoComposeRotationFromTo.h>
#include <Inventor/engines/SoDecomposeRotation.h>
#include <Inventor/engines/SoTransformVec3f.h>
#include <Inventor/nodekits/SoShapeKit.h>
#include <Inventor/nodes/SoFont.h>
#include <Inventor/nodes/SoLineSet.h>
#include <Inventor/nodes/SoMatrixTransform.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/nodes/SoVertexProperty.h>
#include <Inventor/nodekits/SoBaseKit.h>
#endif

#include <Precision.hxx>
#include <Geom_Curve.hxx>
#include <Geom_Line.hxx>
#include <gp_Vec.hxx>
#include <gp_Lin.hxx>
#include <gp_Pnt.hxx>
#include <GeomAPI_ExtremaCurveCurve.hxx>
#include <GeomAPI_ProjectPointOnCurve.hxx>

#include <App/Application.h>
#include <App/Document.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Quantity.h>
#include <Base/Tools.h>
#include <Gui/ArcEngine.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/ViewParams.h>

#include <Mod/Measure/App/Preferences.h>

#include "ViewProviderMeasureAngle.h"


using namespace MeasureGui;
using namespace Measure;

gp_Lin getLine(gp_Vec& vec, gp_Vec& origin)
{
    gp_Pnt tempOrigin;
    tempOrigin.SetXYZ(origin.XYZ());
    return gp_Lin(tempOrigin, gp_Dir(vec));
}

SbMatrix ViewProviderMeasureAngle::getMatrix()
{
    // Code ported from src/Mod/Part/Gui/TaskDimension.cpp

    if (pcObject == nullptr) {
        throw Base::RuntimeError("no DocumentObject");
    }

    Measure::MeasureAngle* measurement = static_cast<Measure::MeasureAngle*>(pcObject);

    if (!measurement->Element1.getValue() || measurement->Element1.getSubValues().empty()) {
        return SbMatrix();
    }
    if (!measurement->Element2.getValue() || measurement->Element2.getSubValues().empty()) {
        return SbMatrix();
    }


    gp_Vec vector1 = measurement->vector1();
    gp_Vec vector2 = measurement->vector2();

    gp_Vec loc1 = measurement->location1();
    gp_Vec loc2 = measurement->location2();

    if (vector1.Magnitude() == 0 || vector2.Magnitude() == 0) {
        return SbMatrix();
    }

    gp_Lin lin1 = getLine(vector1, loc1);
    gp_Lin lin2 = getLine(vector2, loc2);

    SbMatrix dimSys = SbMatrix();
    double radius;

    if (vector1.IsParallel(vector2, Precision::Angular())) {
        // take first point project it onto second vector.
        Handle(Geom_Curve) heapLine2 = new Geom_Line(lin2);
        gp_Pnt tempPoint(loc1.XYZ());

        GeomAPI_ProjectPointOnCurve projection(tempPoint, heapLine2);
        if (projection.NbPoints() < 1) {
            throw Base::RuntimeError("parallel vectors: could not project onto line");
        }
        gp_Vec newPoint2;
        newPoint2.SetXYZ(projection.Point(1).XYZ());

        // if points are colinear, projection doesn't work and returns the same point.
        // In this case we just use the original point.
        if ((newPoint2 - loc1).Magnitude() < Precision::Confusion()) {
            newPoint2 = loc2;
        }

        // now get midpoint between for dim origin.
        gp_Vec point1 = loc1;
        gp_Vec midPointProjection = newPoint2 - point1;
        double distance = midPointProjection.Magnitude();
        midPointProjection.Normalize();
        midPointProjection *= distance / 2.0;

        gp_Vec origin = point1 + midPointProjection;

        // yaxis should be the same as vector1, but doing this to eliminate any potential slop from
        // using precision::angular. If lines are colinear and we have no plane, we can't establish
        // zAxis from crossing. we just the absolute axis.
        gp_Vec xAxis = (point1 - origin).Normalized();
        gp_Vec zAxis;
        if (xAxis.IsParallel(vector1, Precision::Angular())) {
            if (!xAxis.IsParallel(gp_Vec(0.0, 0.0, 1.0), Precision::Angular())) {
                zAxis = gp_Vec(0.0, 0.0, 1.0);
            }
            else {
                zAxis = gp_Vec(0.0, 1.0, 0.0);
            }
        }
        else {
            zAxis = xAxis.Crossed(vector1).Normalized();
        }
        gp_Vec yAxis = zAxis.Crossed(xAxis).Normalized();
        zAxis = xAxis.Crossed(yAxis).Normalized();

        dimSys = SbMatrix(xAxis.X(),
                          yAxis.X(),
                          zAxis.X(),
                          origin.X(),
                          xAxis.Y(),
                          yAxis.Y(),
                          zAxis.Y(),
                          origin.Y(),
                          xAxis.Z(),
                          yAxis.Z(),
                          zAxis.Z(),
                          origin.Z(),
                          0.0,
                          0.0,
                          0.0,
                          1.0);
        dimSys = dimSys.transpose();

        radius = midPointProjection.Magnitude();
    }
    else {
        Handle(Geom_Curve) heapLine1 = new Geom_Line(lin1);
        Handle(Geom_Curve) heapLine2 = new Geom_Line(lin2);

        GeomAPI_ExtremaCurveCurve extrema(heapLine1, heapLine2);

        if (extrema.NbExtrema() < 1) {
            throw Base::RuntimeError("Could not get extrema");
        }

        gp_Pnt extremaPoint1, extremaPoint2, dimensionOriginPoint;
        extrema.Points(1, extremaPoint1, extremaPoint2);
        if (extremaPoint1.Distance(extremaPoint2) < Precision::Confusion()) {
            dimensionOriginPoint = extremaPoint1;
        }
        else {
            // find halfway point in between extrema points for dimension origin.
            gp_Vec vec1(extremaPoint1.XYZ());
            gp_Vec vec2(extremaPoint2.XYZ());
            gp_Vec connection(vec2 - vec1);
            Standard_Real distance = connection.Magnitude();
            connection.Normalize();
            connection *= (distance / 2.0);
            dimensionOriginPoint.SetXYZ((vec1 + connection).XYZ());
        }

        gp_Vec thirdPoint(loc2);
        gp_Vec originVector(dimensionOriginPoint.XYZ());
        gp_Vec extrema2Vector(extremaPoint2.XYZ());
        radius = (loc1 - originVector).Magnitude();
        double legOne = (extrema2Vector - originVector).Magnitude();
        if (legOne > Precision::Confusion()) {
            double legTwo = sqrt(pow(radius, 2) - pow(legOne, 2));
            gp_Vec projectionVector(vector2);
            projectionVector.Normalize();
            projectionVector *= legTwo;
            thirdPoint = extrema2Vector + projectionVector;
            gp_Vec hyp(thirdPoint - originVector);
            hyp.Normalize();
            gp_Vec otherSide(loc1 - originVector);
            otherSide.Normalize();
        }

        gp_Vec xAxis = (loc1 - originVector).Normalized();
        gp_Vec fakeYAxis = (thirdPoint - originVector).Normalized();
        gp_Vec zAxis = (xAxis.Crossed(fakeYAxis)).Normalized();
        gp_Vec yAxis = zAxis.Crossed(xAxis).Normalized();

        dimSys = SbMatrix(xAxis.X(),
                          yAxis.X(),
                          zAxis.X(),
                          dimensionOriginPoint.X(),
                          xAxis.Y(),
                          yAxis.Y(),
                          zAxis.Y(),
                          dimensionOriginPoint.Y(),
                          xAxis.Z(),
                          yAxis.Z(),
                          zAxis.Z(),
                          dimensionOriginPoint.Z(),
                          0.0,
                          0.0,
                          0.0,
                          1.0);

        dimSys = dimSys.transpose();
    }

    return dimSys;
}


PROPERTY_SOURCE(MeasureGui::ViewProviderMeasureAngle, MeasureGui::ViewProviderMeasureBase)


ViewProviderMeasureAngle::ViewProviderMeasureAngle()
{
    sPixmap = "Measurement-Angle";

    // Primary Arc
    Gui::ArcEngine* arcEngine = new Gui::ArcEngine();
    arcEngine->angle.connectFrom(&fieldAngle);

    auto calculatorRadius = new SoCalculator();
    calculatorRadius->A.connectFrom(&pDragger->translation);
    calculatorRadius->expression.setValue("oa=length(A)");
    arcEngine->radius.connectFrom(&calculatorRadius->oa);
    arcEngine->deviation.setValue(0.1f);

    SoCoordinate3* coordinates = new SoCoordinate3();
    coordinates->point.connectFrom(&arcEngine->points);

    SoLineSet* lineSet = new SoLineSet();
    lineSet->vertexProperty.setValue(coordinates);
    lineSet->numVertices.connectFrom(&arcEngine->pointCount);
    lineSet->startIndex.setValue(0);

    pLineSeparator->addChild(lineSet);

    // Secondary Arc, from midpoint to label
    auto engineAngle = new SoCalculator();
    engineAngle->A.connectFrom(&arcEngine->midpoint);
    engineAngle->B.connectFrom(&pLabelTranslation->translation);
    engineAngle->expression.setValue(
        "tA=normalize(A); tB=normalize(B); oa=atan2(tB[1], tB[0])-atan2(tA[1], tA[0])");

    Gui::ArcEngine* arcEngineSecondary = new Gui::ArcEngine();
    arcEngineSecondary->radius.connectFrom(&calculatorRadius->oa);
    arcEngineSecondary->deviation.setValue(0.1f);
    arcEngineSecondary->angle.connectFrom(&engineAngle->oa);

    // Rotate arc
    auto engineRotMidpoint = new SoComposeRotationFromTo();  // absolute angle to midpoint
    engineRotMidpoint->from.setValue(SbVec3f(1.0, 0.0, 0.0));
    engineRotMidpoint->to.connectFrom(&arcEngine->midpoint);

    auto matrixEngine = new SoComposeMatrix();
    matrixEngine->rotation.connectFrom(&engineRotMidpoint->rotation);
    auto transformEngine = new SoTransformVec3f();
    transformEngine->matrix.connectFrom(&matrixEngine->matrix);
    transformEngine->vector.connectFrom(&arcEngineSecondary->points);

    SoCoordinate3* coordinatesSecondary = new SoCoordinate3();
    coordinatesSecondary->point.connectFrom(&transformEngine->point);

    SoLineSet* lineSetSecondary = new SoLineSet();
    lineSetSecondary->vertexProperty.setValue(coordinatesSecondary);
    lineSetSecondary->numVertices.connectFrom(&arcEngineSecondary->pointCount);
    lineSetSecondary->startIndex.setValue(0);

    pLineSeparatorSecondary->addChild(lineSetSecondary);
}


void ViewProviderMeasureAngle::redrawAnnotation()
{
    auto obj = dynamic_cast<Measure::MeasureAngle*>(getMeasureObject());
    double angleDeg = obj->Angle.getValue();
    this->fieldAngle = Base::toRadians(angleDeg);

    // Set matrix
    try {
        SbMatrix matrix = getMatrix();
        pcTransform->setMatrix(matrix);
    }
    catch (const Base::Exception& e) {
        Base::Console().error("Error in ViewProviderMeasureAngle::redrawAnnotation: %s\n",
                              e.what());
        return;
    }

    // Set Label
    setLabelValue(static_cast<Measure::MeasureBase*>(pcObject)->getResultString());
}


//! return the feature as a MeasureAngle
Measure::MeasureAngle* ViewProviderMeasureAngle::getMeasureAngle()
{
    Measure::MeasureAngle* feature = dynamic_cast<Measure::MeasureAngle*>(pcObject);
    if (!feature) {
        throw Base::RuntimeError("Feature not found for ViewProviderMeasureAngle");
    }
    return feature;
}


void ViewProviderMeasureAngle::positionAnno(const Measure::MeasureBase* measureObject)
{
    (void)measureObject;
    setLabelTranslation(SbVec3f(0, 0.1 * getViewScale(), 0));
}
