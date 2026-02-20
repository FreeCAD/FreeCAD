// SPDX-License-Identifier: LGPL-2.1-or-later

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


#include <numbers>
#include <sstream>
#include <QApplication>
#include <Inventor/draggers/SoTranslate2Dragger.h>
#include <Inventor/nodes/SoAnnotation.h>
#include <Inventor/nodes/SoBaseColor.h>
#include <Inventor/nodes/SoCone.h>
#include <Inventor/nodes/SoLightModel.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoFontStyle.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <Inventor/nodes/SoPickStyle.h>
#include <Inventor/nodes/SoText2.h>
#include <Inventor/nodes/SoTranslation.h>
#include <Inventor/nodes/SoSwitch.h>
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
#include <Inventor/nodes/SoScale.h>

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
#include <Gui/Control.h>
#include <Mod/Measure/App/Preferences.h>

#include "SoScreenSpaceScale.h"
#include "TaskMeasure.h"
#include "ViewProviderMeasureAngle.h"

#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS_Shape.hxx>

#include <Mod/Part/App/PartFeature.h>

#include <BRep_Tool.hxx>


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

        dimSys = SbMatrix(
            xAxis.X(),
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
            1.0
        );

        dimSys = dimSys.transpose();
    }
    else {
        gp_Pnt dimensionOriginPoint;
        dimensionOriginPoint.SetCoord(0, 0, 0);
        bool originFound = false;
        gp_Vec thirdPoint;
        gp_Vec adjustedVector1;
        gp_Vec adjustedVector2;
        measurement->getDirections(adjustedVector1, adjustedVector2);

        auto measurmentCase = measurement->measurementCase();

        originFound = measurement->getOrigin(dimensionOriginPoint);

        // need testing before removethis
        if (!originFound) {
            Handle(Geom_Curve) heapLine1 = new Geom_Line(lin1);
            Handle(Geom_Curve) heapLine2 = new Geom_Line(lin2);

            GeomAPI_ExtremaCurveCurve extrema(heapLine1, heapLine2);

            if (extrema.NbExtrema() < 1) {
                throw Base::RuntimeError("Could not get extrema");
            }

            gp_Pnt extremaPoint1, extremaPoint2;
            extrema.Points(1, extremaPoint1, extremaPoint2);

            bool linesIntersect = extremaPoint1.Distance(extremaPoint2) < Precision::Confusion();

            if (linesIntersect) {
                dimensionOriginPoint = extremaPoint1;
            }
            else {
                dimensionOriginPoint.SetXYZ(loc2.XYZ());
            }

            gp_Vec originVector(dimensionOriginPoint.XYZ());

            if (linesIntersect) {
                gp_Vec extrema2Vector(extremaPoint2.XYZ());
                double radiusCalc = (loc1 - originVector).Magnitude();
                double legOne = (extrema2Vector - originVector).Magnitude();
                if (legOne > Precision::Confusion() && radiusCalc > legOne) {
                    double legTwo = sqrt(pow(radiusCalc, 2) - pow(legOne, 2));
                    gp_Vec projectionVector(adjustedVector2);
                    projectionVector.Normalize();
                    projectionVector *= legTwo;
                    thirdPoint = extrema2Vector + projectionVector;
                }
                else {
                    thirdPoint = originVector + adjustedVector2.Normalized();
                }
            }
            else {
                thirdPoint = originVector + adjustedVector2.Normalized();
            }
        }

        gp_Vec originVector(dimensionOriginPoint.XYZ());
        gp_Vec xAxis = (loc1 - originVector).Normalized();
        gp_Vec zAxis;
        gp_Vec yAxis;

        // need to review this but works
        if (originFound) {
            if (measurmentCase == MeasureAngle::EdgeEdge) {
                xAxis = adjustedVector1.Normalized();
            }
            if (measurmentCase == MeasureAngle::FaceFace) {
                zAxis = adjustedVector1.Crossed(adjustedVector2);
            }
            else {
                zAxis = adjustedVector2.Crossed(adjustedVector1);
            }
            zAxis.Normalize();
            yAxis = zAxis.Crossed(xAxis).Normalized();
            xAxis = zAxis.Crossed(yAxis).Normalized();
        }
        else {
            gp_Vec fakeYAxis = (thirdPoint - originVector).Normalized();
            zAxis = (xAxis.Crossed(fakeYAxis)).Normalized();
            yAxis = zAxis.Crossed(xAxis).Normalized();
        }

        dimSys = SbMatrix(
            xAxis.X(),
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
            1.0
        );

        dimSys = dimSys.transpose();
    }

    return dimSys;
}


PROPERTY_SOURCE(MeasureGui::ViewProviderMeasureAngle, MeasureGui::ViewProviderMeasureBase)


ViewProviderMeasureAngle::ViewProviderMeasureAngle()
{
    sPixmap = "Measurement-Angle";
    const char* agroup = "Appearance";
    ADD_PROPERTY_TYPE(IsFlipped, (false), agroup, App::Prop_None, "Is flipped(vertically opposite)");

    // Visuals node
    auto pVisualsSep = new SoSeparator();

    // Primary Arc with rotation transform (sector rotation)
    auto pArcVisCalc = new SoComposeRotation();
    pArcVisCalc->axis.setValue(0, 0, 1);
    pArcVisCalc->angle.connectFrom(&sectorArcRotation);

    auto pArcTransform = new SoTransform();
    pArcTransform->rotation.connectFrom(&pArcVisCalc->rotation);
    pVisualsSep->addChild(pArcTransform);

    pLineSeparator->addChild(pVisualsSep);


    auto pArcGeomNode = new SoSeparator();

    // ========================== ARC ==========================
    Gui::ArcEngine* arcEngine = new Gui::ArcEngine();
    arcEngine->angle.connectFrom(&fieldAngle);

    auto calculatorRadius = new SoCalculator();
    calculatorRadius->A.connectFrom(&pDragger->translation);
    calculatorRadius->expression.setValue("oa=length(A)");
    arcEngine->radius.connectFrom(&calculatorRadius->oa);
    arcEngine->deviation.setValue(0.1f);

    SoCoordinate3* coordinates = new SoCoordinate3();
    coordinates->point.connectFrom(&arcEngine->points);
    pArcGeomNode->addChild(coordinates);

    SoLineSet* lineSet = new SoLineSet();
    lineSet->numVertices.connectFrom(&arcEngine->pointCount);
    lineSet->startIndex.setValue(0);

    pArcGeomNode->addChild(lineSet);

    // Secondary Arc, from midpoint to label
    auto engineAngle = new SoCalculator();
    engineAngle->A.connectFrom(&arcEngine->midpoint);
    engineAngle->B.connectFrom(&pLabelTranslation->translation);
    engineAngle->c.connectFrom(&isArcFlipped);
    // as for now we just have two sector so this trick works
    engineAngle->expression.setValue(
        "tA=normalize(A); tB=(c>0)?-normalize(B):normalize(B); "
        "oa=atan2(tB[1], tB[0])-atan2(tA[1], tA[0])"
    );

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

    pArcGeomNode->addChild(lineSetSecondary);
    pVisualsSep->addChild(pArcGeomNode);


    // ========================== ARROWS ==========================

    auto pArrowGeomNode = new SoSeparator();
    // single arrow for arc
    auto pArrowNode = new SoSeparator();

    auto pLightModel = new SoLightModel();
    pLightModel->model.setValue(SoLightModel::BASE_COLOR);
    pArrowNode->addChild(pLightModel);

    // apply zoom based scaling
    auto pScreenSpaceScale = new SoScreenSpaceScale();
    pScreenSpaceScale->referenceSize.setValue(1.0f);  // 1 unit = 1 pixel
    pArrowNode->addChild(pScreenSpaceScale);

    // scale arrows based on arc length prevent overlap
    auto pScaleCalc = new SoCalculator();
    pScaleCalc->a.connectFrom(&calculatorRadius->oa);
    pScaleCalc->b.connectFrom(&fieldAngle);
    pScaleCalc->c.connectFrom(&fieldArrowHeight);
    pScaleCalc->d.connectFrom(&pScreenSpaceScale->finalScale);
    pScaleCalc->expression.setValue("ta = (a*b)/(3*c*d);tb = ta<1?ta:1; oA = vec3f(tb, tb, tb)");

    auto pScale = new SoScale();
    pScale->scaleFactor.connectFrom(&pScaleCalc->oA);
    pArrowNode->addChild(pScale);

    // Offset cone tip
    auto pTipOffsetCalc = new SoCalculator();
    pTipOffsetCalc->a.connectFrom(&fieldArrowHeight);
    pTipOffsetCalc->expression.setValue("oA = vec3f(0, -a*0.5, 0)");

    auto pTipOffsetTrans = new SoTranslation();
    pTipOffsetTrans->translation.connectFrom(&pTipOffsetCalc->oA);
    pArrowNode->addChild(pTipOffsetTrans);

    // Create cone shape as Arrow
    auto pCone = new SoCone();
    pCone->bottomRadius.connectFrom(&fieldArrowRadius);
    pCone->height.connectFrom(&fieldArrowHeight);
    pArrowNode->addChild(pCone);

    auto pLeftArrowSep = new SoSeparator();
    auto pLeftArrowTrans = new SoTransform();
    pLeftArrowSep->addChild(pLeftArrowTrans);
    pLeftArrowSep->addChild(pArrowNode);
    pArrowGeomNode->addChild(pLeftArrowSep);

    auto pRightArrowSep = new SoSeparator();
    auto pRightArrowTrans = new SoTransform();
    pRightArrowSep->addChild(pRightArrowTrans);
    pRightArrowSep->addChild(pArrowNode);
    pArrowGeomNode->addChild(pRightArrowSep);

    pLeftArrowTrans->rotation.setValue(SbRotation(SbVec3f(0, 0, 1), std::numbers::pi));

    auto rightRotCalc = new SoComposeRotation();
    rightRotCalc->axis.setValue(0, 0, 1);
    rightRotCalc->angle.connectFrom(&fieldAngle);
    pRightArrowTrans->rotation.connectFrom(&rightRotCalc->rotation);

    pVisualsSep->addChild(pArrowGeomNode);


    // ========================== Normals ==========================

    // normal for faces
    auto pNormalsSwitch = new SoSwitch();
    pNormalsSwitch->whichChild.connectFrom(&visualMode);

    auto pNormalsStandardSep = new SoSeparator();

    auto pNormalsCoords = new SoCoordinate3();
    auto pNormalsLines = new SoLineSet();

    auto pNormalsStandardStyle = new SoDrawStyle();
    pNormalsStandardStyle->style.setValue(SoDrawStyle::LINES);
    pNormalsStandardStyle->lineWidth.setValue(0.8f);

    // Arrowtip for normal lines
    auto tipCalc = new SoCalculator();
    tipCalc->a.connectFrom(&calculatorRadius->oa);
    tipCalc->b.connectFrom(&fieldAngle);

    // oC is used as the local origin
    tipCalc->expression.setValue("oA = vec3f(a, 0, 0); oB = vec3f(a*cos(b), a*sin(b), 0);");

    // set arrow position
    pLeftArrowTrans->translation.connectFrom(&tipCalc->oA);
    pRightArrowTrans->translation.connectFrom(&tipCalc->oB);

    // Calculator normal start points
    auto pNormalsStartPointCalc = new SoCalculator();
    pNormalsStartPointCalc->A.connectFrom(&normalStartPoint1);
    pNormalsStartPointCalc->B.connectFrom(&normalStartPoint2);
    pNormalsStartPointCalc->c.connectFrom(&sectorArcRotation);
    pNormalsStartPointCalc->expression.setValue(
        "oA = (c > 0) ? vec3f(-A[0], -A[1], A[2]) : A; "
        "oB = (c > 0) ? vec3f(-B[0], -B[1], B[2]) : B"
    );

    auto normalsConcat = new SoConcatenate(SoMFVec3f::getClassTypeId());

    normalsConcat->input[0]->connectFrom(&pNormalsStartPointCalc->oA);
    normalsConcat->input[1]->connectFrom(&tipCalc->oA);

    normalsConcat->input[2]->connectFrom(&pNormalsStartPointCalc->oB);
    normalsConcat->input[3]->connectFrom(&tipCalc->oB);

    pNormalsCoords->point.connectFrom(normalsConcat->output);

    pNormalsLines->numVertices.setNum(2);
    pNormalsLines->numVertices.set1Value(0, 2);
    pNormalsLines->numVertices.set1Value(1, 2);

    pNormalsStandardSep->addChild(pArcTransform);
    pNormalsStandardSep->addChild(pNormalsStandardStyle);
    pNormalsStandardSep->addChild(pNormalsCoords);
    pNormalsStandardSep->addChild(pNormalsLines);

    pNormalsSwitch->addChild(pNormalsStandardSep);


    auto pNormalsImgSep = new SoSeparator();
    auto pNormalsStyle = new SoDrawStyle();
    pNormalsStyle->style.setValue(SoDrawStyle::LINES);
    pNormalsStyle->lineWidth.setValue(1.0f);
    pNormalsStyle->linePattern.setValue(0xF0F0);
    auto pNormalsColor = new SoBaseColor();
    pNormalsColor->rgb.setValue(0, 0, 1);

    pNormalsImgSep->addChild(pNormalsStyle);
    pNormalsImgSep->addChild(pNormalsColor);

    auto pNormalsImgCoords = new SoCoordinate3();
    auto pNormalsImgLines = new SoLineSet();

    auto pNormalsImgConcat = new SoConcatenate(SoMFVec3f::getClassTypeId());

    auto pAxisCalc = new SoCalculator();
    pAxisCalc->A.connectFrom(&element1Location);
    pAxisCalc->B.connectFrom(&element2Location);
    pAxisCalc->expression.setValue(
        "oA = vec3f(0, 0, 0); "
        "oB = vec3f(A[0], A[1], 0); "
        "oC = vec3f(B[0], B[1], 0)"
    );
    auto pNormalsRotPointCalc = new SoCalculator();
    pNormalsRotPointCalc->A.connectFrom(&tipCalc->oA);
    pNormalsRotPointCalc->B.connectFrom(&tipCalc->oB);
    pNormalsRotPointCalc->c.connectFrom(&sectorArcRotation);
    pNormalsRotPointCalc->expression.setValue(
        "oA = (c > 0) ? -A : A; "
        "oB = (c > 0) ? -B : B"
    );

    pNormalsImgConcat->input[0]->connectFrom(&pNormalsRotPointCalc->oA);
    pNormalsImgConcat->input[1]->connectFrom(&pAxisCalc->oB);

    pNormalsImgConcat->input[2]->connectFrom(&pAxisCalc->oB);
    pNormalsImgConcat->input[3]->connectFrom(&element1Location);

    pNormalsImgConcat->input[4]->connectFrom(&pNormalsRotPointCalc->oB);
    pNormalsImgConcat->input[5]->connectFrom(&pAxisCalc->oC);

    pNormalsImgConcat->input[6]->connectFrom(&pAxisCalc->oC);
    pNormalsImgConcat->input[7]->connectFrom(&element2Location);

    pNormalsImgCoords->point.connectFrom(pNormalsImgConcat->output);

    pNormalsImgLines->numVertices.setNum(4);
    pNormalsImgLines->numVertices.set1Value(0, 2);
    pNormalsImgLines->numVertices.set1Value(1, 2);
    pNormalsImgLines->numVertices.set1Value(2, 2);
    pNormalsImgLines->numVertices.set1Value(3, 2);

    pNormalsImgSep->addChild(pNormalsImgCoords);
    pNormalsImgSep->addChild(pNormalsImgLines);

    pNormalsSwitch->addChild(pNormalsImgSep);

    pLineSeparator->addChild(pNormalsSwitch);
}

void ViewProviderMeasureAngle::redrawAnnotation()
{
    auto obj = getMeasureAngle();
    double angleDeg = obj->Angle.getValue();
    this->fieldAngle = Base::toRadians(angleDeg);

    // update arrow sizes from preferences, (same arrow size for all)
    ArrowHeight.setValue(Measure::Preferences::defaultArrowHeight());
    ArrowRadius.setValue(Measure::Preferences::defaultArrowRadius());

    // Set matrix
    SbMatrix matrix;
    try {
        matrix = getMatrix();
        pcTransform->setMatrix(matrix);
    }
    catch (const Base::Exception& e) {
        Base::Console().error("Error in ViewProviderMeasureAngle::redrawAnnotation: %s\n", e.what());
        return;
    }

    // imaginary origin case
    if (obj->isImgOrigin()) {
        gp_Vec loc1 = obj->location1();
        gp_Vec loc2 = obj->location2();
        SbMatrix invMatrix = matrix.inverse();
        SbVec3f globalLoc1(loc1.X(), loc1.Y(), loc1.Z());
        SbVec3f globalLoc2(loc2.X(), loc2.Y(), loc2.Z());

        SbVec3f localLoc1, localLoc2;
        invMatrix.multVecMatrix(globalLoc1, localLoc1);
        invMatrix.multVecMatrix(globalLoc2, localLoc2);

        visualMode.setValue(1);

        element1Location.setValue(localLoc1);
        element2Location.setValue(localLoc2);
    }
    else {
        visualMode.setValue(0);
    }
    normalStartPoint1.setValue(0, 0, 0);
    normalStartPoint2.setValue(0, 0, 0);

    // Set Label
    setLabelValue(static_cast<Measure::MeasureBase*>(pcObject)->getResultString());

    bool flipped = IsFlipped.getValue();
    isArcFlipped.setValue(flipped);
    sectorArcRotation.setValue(flipped ? M_PI : 0.0f);
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
    // for imgOrigin, the initial radius is set to the center of the two obj
    auto obj = getMeasureAngle();
    if (obj->isImgOrigin()) {
        gp_Vec loc1 = obj->location1();
        gp_Vec loc2 = obj->location2();
        if (loc1.Magnitude() < Precision::Confusion() || loc2.Magnitude() < Precision::Confusion()) {
            return;
        }

        SbMatrix invMatrix = getMatrix().inverse();

        SbVec3f globalLoc1(loc1.X(), loc1.Y(), loc1.Z());
        SbVec3f globalLoc2(loc2.X(), loc2.Y(), loc2.Z());

        SbVec3f localLoc1, localLoc2;
        invMatrix.multVecMatrix(globalLoc1, localLoc1);
        invMatrix.multVecMatrix(globalLoc2, localLoc2);

        // arc radius = distance to the center of the two edge locations
        SbVec3f center = (localLoc1 + localLoc2) / 2.0f;
        center[2] = 0.0f;
        setLabelTranslation(center);
        return;
    }

    setLabelTranslation(SbVec3f(0, 0.1 * getViewScale(), 0));
}

void ViewProviderMeasureAngle::onLabelMoved()
{
    if (!Gui::Control().activeDialog()
        || !dynamic_cast<MeasureGui::TaskMeasure*>(Gui::Control().activeDialog())) {
        return;
    }
    SbVec3f trans = pDragger->translation.getValue();

    // The 4 sectors are defined by the two intersecting lines
    float measuredAngle = fieldAngle.getValue();

    // Get dragger angle in local coordinate system
    float draggerAngle = std::atan2(trans[1], trans[0]);
    if (draggerAngle < 0) {
        draggerAngle += 2.0f * M_PI;
    }

    float theta = measuredAngle;
    if (theta > M_PI) {
        theta = 2.0f * M_PI - theta;
    }

    // sector 1: between 0 and θ
    if (draggerAngle >= 0 && draggerAngle < theta) {
        sectorArcRotation.setValue(0);
        isArcFlipped.setValue(false);
    }
    // sector 3: between π and π+θ
    else if (draggerAngle >= M_PI && draggerAngle < M_PI + theta) {
        sectorArcRotation.setValue(M_PI);
        isArcFlipped.setValue(true);
    }
}

void ViewProviderMeasureAngle::onLabelMoveEnd()
{
    if (!Gui::Control().activeDialog()
        || !dynamic_cast<MeasureGui::TaskMeasure*>(Gui::Control().activeDialog())) {
        return;
    }
    IsFlipped.setValue(isArcFlipped.getValue());
}

void ViewProviderMeasureAngle::onChanged(const App::Property* prop)
{
    if (prop == &IsFlipped) {
        bool flipped = IsFlipped.getValue();
        isArcFlipped.setValue(flipped);
        sectorArcRotation.setValue(flipped ? M_PI : 0);
    }

    ViewProviderMeasureBase::onChanged(prop);
}
