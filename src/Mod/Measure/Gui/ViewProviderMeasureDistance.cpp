// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2023 David Friedli <david[at]friedli-be.ch>             *
 *   Copyright (c) 2013 Thomas Anderson <blobfish[at]gmx.com>              *
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#include <algorithm>
#include <cmath>
#include <sstream>
#include <QApplication>
#include <Inventor/engines/SoCalculator.h>
#include <Inventor/engines/SoConcatenate.h>
#include <Inventor/engines/SoComposeRotation.h>
#include <Inventor/engines/SoComposeRotationFromTo.h>
#include <Inventor/engines/SoComposeVec3f.h>
#include <Inventor/engines/SoDecomposeVec3f.h>
#include <Inventor/nodes/SoAnnotation.h>
#include <Inventor/nodes/SoBaseColor.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoFontStyle.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <Inventor/nodes/SoPickStyle.h>
#include <Inventor/nodes/SoText2.h>
#include <Inventor/nodes/SoTranslation.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoCone.h>
#include <Inventor/elements/SoFocalDistanceElement.h>
#include <Inventor/elements/SoViewportRegionElement.h>
#include <Inventor/elements/SoViewVolumeElement.h>
#include <Inventor/misc/SoState.h>
#include <Inventor/nodes/SoResetTransform.h>
#include <Inventor/nodes/SoVertexProperty.h>
#include <Inventor/nodes/SoNodes.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoSearchAction.h>
#include <Inventor/draggers/SoTranslate2Dragger.h>
#include <Inventor/nodes/SoCallback.h>


#include <App/Document.h>
#include <Base/BaseClass.h>
#include <Base/Console.h>
#include <Base/Quantity.h>
#include <Mod/Measure/App/Preferences.h>

#include "ViewProviderMeasureDistance.h"
#include "Gui/Application.h"
#include <Gui/Command.h>
#include "Gui/Document.h"
#include "Gui/ViewParams.h"
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>


using namespace Gui;
using namespace MeasureGui;
using namespace Measure;

PROPERTY_SOURCE(MeasureGui::ViewProviderMeasureDistance, MeasureGui::ViewProviderMeasureBase)

SO_KIT_SOURCE(MeasureGui::DimensionLinear)

void MeasureGui::DimensionLinear::initClass()
{
    SO_KIT_INIT_CLASS(DimensionLinear, SoSeparatorKit, "SeparatorKit");
}

MeasureGui::DimensionLinear::DimensionLinear()
{
    SO_KIT_CONSTRUCTOR(MeasureGui::DimensionLinear);

    SO_KIT_ADD_CATALOG_ENTRY(transformation, SoTransform, true, topSeparator, "", true);
    SO_KIT_ADD_CATALOG_ENTRY(annotate, SoAnnotation, true, topSeparator, "", true);
    SO_KIT_ADD_CATALOG_ENTRY(leftArrow, SoShapeKit, true, topSeparator, "", true);
    SO_KIT_ADD_CATALOG_ENTRY(rightArrow, SoShapeKit, true, topSeparator, "", true);
    SO_KIT_ADD_CATALOG_ENTRY(line, SoShapeKit, true, annotate, "", true);
    SO_KIT_ADD_CATALOG_ENTRY(textSep, SoSeparator, true, annotate, "", true);

    SO_KIT_INIT_INSTANCE();

    SO_NODE_ADD_FIELD(rotate, (1.0, 0.0, 0.0, 0.0));  // position orientation of the dimension.
    SO_NODE_ADD_FIELD(length, (1.0));                 // turns into dimension length
    SO_NODE_ADD_FIELD(origin, (0.0, 0.0, 0.0));       // static
    SO_NODE_ADD_FIELD(text, ("test"));                // dimension text
    SO_NODE_ADD_FIELD(dColor, (1.0, 0.0, 0.0));       // dimension color.
    SO_NODE_ADD_FIELD(backgroundColor, (1.0, 1.0, 1.0));
    SO_NODE_ADD_FIELD(showArrows, (true));  // display arrowheads at dimension endpoints
    SO_NODE_ADD_FIELD(fontSize, (12.0));    // size of the dimension font
}

MeasureGui::DimensionLinear::~DimensionLinear()
{}

SbBool MeasureGui::DimensionLinear::affectsState() const
{
    return false;
}

void MeasureGui::DimensionLinear::setupDimension()
{
    // make unpickable
    SoPickStyle* ps = static_cast<SoPickStyle*>(getPart("pickStyle", true));
    if (ps) {
        ps->style = SoPickStyle::UNPICKABLE;
    }

    // transformation
    SoTransform* trans = static_cast<SoTransform*>(getPart("transformation", true));
    trans->translation.connectFrom(&point1);
    // build engine for vector subtraction and length.
    SoCalculator* hyp = new SoCalculator();
    hyp->A.connectFrom(&point1);
    hyp->B.connectFrom(&point2);
    hyp->expression.set1Value(0, "oA = B-A");
    hyp->expression.set1Value(1, "oB = normalize(oA)");
    hyp->expression.set1Value(2, "oa = length(oA)");
    length.connectFrom(&hyp->oa);

    // build engine for rotation.
    SoComposeRotationFromTo* rotationEngine = new SoComposeRotationFromTo();
    rotationEngine->from.setValue(SbVec3f(1.0, 0.0, 0.0));
    rotationEngine->to.connectFrom(&hyp->oB);
    trans->rotation.connectFrom(&rotationEngine->rotation);

    // color
    SoMaterial* material = new SoMaterial;
    material->diffuseColor.connectFrom(&dColor);
    material->transparency.setValue(0.0f);

    SoComposeVec3f* vec = new SoComposeVec3f;
    vec->x.connectFrom(&length);
    vec->y.setValue(0.0);
    vec->z.setValue(0.0);

    // Proportional arrowhead sizing: height = 6% of length (min 0.3), radius = 2.5% (min 0.12).
    auto* sizingCalc = new SoCalculator();
    sizingCalc->a.connectFrom(&length);
    sizingCalc->expression.set1Value(0, "oa = (a * 0.06 > 0.3) ? a * 0.06 : 0.3");
    sizingCalc->expression.set1Value(1, "ob = (a * 0.025 > 0.12) ? a * 0.025 : 0.12");

    auto* rightCone = new SoCone();
    rightCone->height.connectFrom(&sizingCalc->oa);
    rightCone->bottomRadius.connectFrom(&sizingCalc->ob);

    // Offset each cone by half its height so its tip touches the dimension endpoint.
    auto* rightPosCalc = new SoCalculator();
    rightPosCalc->a.connectFrom(&length);
    rightPosCalc->b.connectFrom(&sizingCalc->oa);
    rightPosCalc->expression.set1Value(0, "oA = vec3f(a - b * 0.5, 0.0, 0.0)");

    auto* rightTransform = new SoTransform();
    rightTransform->translation.connectFrom(&rightPosCalc->oA);
    // Rotate the +Y cone to point in +X (right endpoint direction).
    rightTransform->rotation.setValue(SbRotation(SbVec3f(0.0f, 1.0f, 0.0f), SbVec3f(1.0f, 0.0f, 0.0f)));

    auto* rightArrowSep = new SoSeparator();
    rightArrowSep->addChild(material);
    rightArrowSep->addChild(rightTransform);
    rightArrowSep->addChild(rightCone);

    auto* leftCone = new SoCone();
    leftCone->height.connectFrom(&sizingCalc->oa);
    leftCone->bottomRadius.connectFrom(&sizingCalc->ob);

    auto* leftPosCalc = new SoCalculator();
    leftPosCalc->b.connectFrom(&sizingCalc->oa);
    leftPosCalc->expression.set1Value(0, "oA = vec3f(b * 0.5, 0.0, 0.0)");

    auto* leftTransform = new SoTransform();
    leftTransform->translation.connectFrom(&leftPosCalc->oA);
    // Rotate the +Y cone to point in -X (left endpoint direction).
    leftTransform->rotation.setValue(SbRotation(SbVec3f(0.0f, 1.0f, 0.0f), SbVec3f(-1.0f, 0.0f, 0.0f)));

    auto* leftArrowSep = new SoSeparator();
    leftArrowSep->addChild(material);
    leftArrowSep->addChild(leftTransform);
    leftArrowSep->addChild(leftCone);

    // SoSwitch uses -3 (SO_SWITCH_ALL) to show and -1 (SO_SWITCH_NONE) to hide;
    // a SoCalculator bridges the bool showArrows field to the integer whichChild.
    auto* arrowVisCalc = new SoCalculator();
    arrowVisCalc->a.connectFrom(&showArrows);
    arrowVisCalc->expression.set1Value(0, "oa = (a > 0.5) ? -3.0 : -1.0");

    auto* arrowSwitch = new SoSwitch();
    arrowSwitch->whichChild.connectFrom(&arrowVisCalc->oa);
    arrowSwitch->addChild(rightArrowSep);
    arrowSwitch->addChild(leftArrowSep);

    // Add to topSeparator (not annotate) so delta arrows stay in the measurement frame's 3D space.
    auto* topGroup = static_cast<SoGroup*>(getPart("topSeparator", true));
    if (topGroup) {
        topGroup->addChild(arrowSwitch);
    }

    // line
    SoConcatenate* catEngine = new SoConcatenate(SoMFVec3f::getClassTypeId());
    // don't know how to get around having this dummy origin. cat engine wants to connectfrom?
    catEngine->input[0]->connectFrom(&origin);
    catEngine->input[1]->connectFrom(&vec->vector);

    SoVertexProperty* lineVerts = new SoVertexProperty;
    lineVerts->vertex.connectFrom(catEngine->output);

    int lineVertexMap[] = {0, 1};
    int lineVertexMapSize(sizeof(lineVertexMap) / sizeof(int));
    SoIndexedLineSet* line = new SoIndexedLineSet;
    line->vertexProperty = lineVerts;
    line->coordIndex.setValues(0, lineVertexMapSize, lineVertexMap);

    setPart("line.shape", line);
    setPart("line.material", material);

    // text
    SoSeparator* textSep = static_cast<SoSeparator*>(getPart("textSep", true));
    if (!textSep) {
        return;
    }

    textSep->addChild(material);

    SoCalculator* textVecCalc = new SoCalculator();
    textVecCalc->A.connectFrom(&vec->vector);
    textVecCalc->a.connectFrom(&length);
    // Label at midpoint of the dimension line, offset 15% below the axis.
    textVecCalc->expression.set1Value(0, "oA = (A / 2) + vec3f(0.0, a * -0.15, 0.0)");

    SoTransform* textTransform = new SoTransform();
    textTransform->translation.connectFrom(&textVecCalc->oA);
    textSep->addChild(textTransform);

    auto textNode = new SoFrameLabel();
    textNode->justification = SoText2::CENTER;
    textNode->string.connectFrom(&text);
    textNode->textColor.connectFrom(&dColor);
    textNode->backgroundColor.connectFrom(&backgroundColor);
    textNode->size.connectFrom(&fontSize);
    textNode->name.setValue("Helvetica");
    textSep->addChild(textNode);

    // this prevents the 2d text from screwing up the bounding box for a viewall
    SoResetTransform* rTrans = new SoResetTransform;
    rTrans->whatToReset = SoResetTransform::BBOX;
    textSep->addChild(rTrans);
}


SbMatrix ViewProviderMeasureDistance::getMatrix()
{
    if (!pcObject) {
        return {};
    }

    auto prop1 = freecad_cast<App::PropertyVector*>(pcObject->getPropertyByName("Position1"));
    auto prop2 = freecad_cast<App::PropertyVector*>(pcObject->getPropertyByName("Position2"));

    if (!prop1 || !prop2) {
        return {};
    }

    auto vec1 = prop1->getValue();
    auto vec2 = prop2->getValue();

    const double tolerance(10.0e-6);
    SbVec3f origin = toSbVec3f((vec2 + vec1) / 2);
    Base::Vector3d localXAxis = (vec2 - vec1).Normalize();
    Base::Vector3d localYAxis = getTextDirection(localXAxis, tolerance).Normalize();

    // X and Y axis have to be 90° to each other
    assert(fabs(localYAxis.Dot(localXAxis)) < tolerance);
    // Cross product order matters: X×Y gives the correct outward normal for a right-handed frame.
    Base::Vector3d localZAxis = localXAxis.Cross(localYAxis).Normalize();

    SbMatrix matrix = SbMatrix(
        localXAxis.x,
        localXAxis.y,
        localXAxis.z,
        0,
        localYAxis.x,
        localYAxis.y,
        localYAxis.z,
        0,
        localZAxis.x,
        localZAxis.y,
        localZAxis.z,
        0,
        // 0,0,0,1
        origin[0],
        origin[1],
        origin[2],
        1
    );

    return matrix;
}


//! calculate a good direction from the elements being measured to the annotation text based on the
//! layout of the elements and its relationship with the cardinal axes and the view direction.
//! elementDirection is expected to be a normalized vector. an example of an elementDirection would
//! be the vector from the start of a line to the end.
Base::Vector3d ViewProviderMeasureDistance::getTextDirection(
    Base::Vector3d elementDirection,
    double tolerance
)
{
    const Base::Vector3d stdX(1.0, 0.0, 0.0);
    const Base::Vector3d stdY(0.0, 1.0, 0.0);
    const Base::Vector3d stdZ(0.0, 0.0, 1.0);

    Base::Vector3d textDirection = elementDirection.Cross(stdX);
    if (textDirection.Length() < tolerance) {
        textDirection = elementDirection.Cross(stdY);
    }
    if (textDirection.Length() < tolerance) {
        textDirection = elementDirection.Cross(stdZ);
    }
    textDirection.Normalize();
    if (textDirection.Dot(stdZ) < 0.0) {
        textDirection = textDirection * -1.0;
    }

    return textDirection.Normalize();
}


ViewProviderMeasureDistance::ViewProviderMeasureDistance()
{
    sPixmap = "Measurement-Distance";

    ADD_PROPERTY_TYPE(
        ShowDelta,
        (false),
        "Appearance",
        App::Prop_None,
        "Display the X, Y and Z components of the distance"
    );

    ADD_PROPERTY_TYPE(
        ShowArrows,
        (true),
        "Appearance",
        App::Prop_None,
        "Display arrowheads at the measurement endpoints"
    );

    ADD_PROPERTY_TYPE(
        ArrowSize,
        (1.0f),
        "Appearance",
        App::Prop_None,
        "Scale factor applied to arrowhead dimensions; 1.0 is the default size"
    );

    pCoords = new SoCoordinate3();
    pCoords->ref();
    pCoords->point.setNum(2);

    static const int32_t lineIndices[] = {0, 1, -1};
    pLines = new SoIndexedLineSet();
    pLines->ref();
    pLines->coordIndex.setNum(3);
    pLines->coordIndex.setValues(0, 3, lineIndices);

    pLineSeparator->addChild(pCoords);
    pLineSeparator->addChild(pLines);

    // SoConcatenate keeps the leader line endpoints live as the label moves.
    auto* leaderCatEngine = new SoConcatenate(SoMFVec3f::getClassTypeId());
    auto* leaderOriginNode = new SoCoordinate3();
    leaderOriginNode->point.set1Value(0, SbVec3f(0.0f, 0.0f, 0.0f));
    leaderCatEngine->input[0]->connectFrom(&leaderOriginNode->point);
    leaderCatEngine->input[1]->connectFrom(&pLabelTranslation->translation);

    auto* leaderVerts = new SoVertexProperty();
    leaderVerts->vertex.connectFrom(leaderCatEngine->output);

    static const int32_t leaderIdx[] = {0, 1, -1};
    auto* leaderLine = new SoIndexedLineSet();
    leaderLine->vertexProperty = leaderVerts;
    leaderLine->coordIndex.setValues(0, 3, leaderIdx);

    auto* leaderDrawStyle = new SoDrawStyle();
    leaderDrawStyle->lineWidth.setValue(1.0f);

    auto* leaderSep = new SoSeparator();
    leaderSep->addChild(leaderOriginNode);
    leaderSep->addChild(leaderDrawStyle);
    leaderSep->addChild(leaderLine);
    pLineSeparator->addChild(leaderSep);


    // SoAnnotation draws after all opaque geometry.
    auto* arrowAnnotation = new SoAnnotation();

    auto* arrowPickStyle = new SoPickStyle();
    arrowPickStyle->style = SoPickStyle::UNPICKABLE;
    arrowAnnotation->addChild(arrowPickStyle);

    auto* arrowCb = new SoCallback();
    arrowCb->setCallback(arrowSizeCallback, this);
    arrowAnnotation->addChild(arrowCb);

    pArrowSwitch = new SoSwitch();
    pArrowSwitch->ref();
    pArrowSwitch->whichChild.setValue(SO_SWITCH_ALL);
    pArrowSwitch->addChild(arrowAnnotation);
    pGlobalSeparator->addChild(pArrowSwitch);


    // Delta Dimensions
    auto decomposedPosition1 = new SoDecomposeVec3f();
    decomposedPosition1->vector.connectFrom(&fieldPosition1);
    auto decomposedPosition2 = new SoDecomposeVec3f();
    decomposedPosition2->vector.connectFrom(&fieldPosition2);

    auto composeVecDelta1 = new SoComposeVec3f();
    composeVecDelta1->x.connectFrom(&decomposedPosition2->x);
    composeVecDelta1->y.connectFrom(&decomposedPosition1->y);
    composeVecDelta1->z.connectFrom(&decomposedPosition1->z);

    auto composeVecDelta2 = new SoComposeVec3f();
    composeVecDelta2->x.connectFrom(&decomposedPosition2->x);
    composeVecDelta2->y.connectFrom(&decomposedPosition2->y);
    composeVecDelta2->z.connectFrom(&decomposedPosition1->z);

    SbColor deltaAxisColors[3];
    {
        float t = 0.0f;
        deltaAxisColors[0].setPackedValue(ViewParams::instance()->getAxisXColor(), t);
        deltaAxisColors[1].setPackedValue(ViewParams::instance()->getAxisYColor(), t);
        deltaAxisColors[2].setPackedValue(ViewParams::instance()->getAxisZColor(), t);
    }

    auto dimDeltaX = new MeasureGui::DimensionLinear();
    dimDeltaX->point1.connectFrom(&fieldPosition1);
    dimDeltaX->point2.connectFrom(&composeVecDelta1->vector);
    dimDeltaX->setupDimension();
    dimDeltaX->dColor.setValue(deltaAxisColors[0]);
    dimDeltaX->fontSize.connectFrom(&fieldFontSize);

    auto dimDeltaY = new MeasureGui::DimensionLinear();
    dimDeltaY->point1.connectFrom(&composeVecDelta1->vector);
    dimDeltaY->point2.connectFrom(&composeVecDelta2->vector);
    dimDeltaY->setupDimension();
    dimDeltaY->dColor.setValue(deltaAxisColors[1]);
    dimDeltaY->fontSize.connectFrom(&fieldFontSize);

    auto dimDeltaZ = new MeasureGui::DimensionLinear();
    dimDeltaZ->point1.connectFrom(&fieldPosition2);
    dimDeltaZ->point2.connectFrom(&composeVecDelta2->vector);
    dimDeltaZ->setupDimension();
    dimDeltaZ->dColor.setValue(deltaAxisColors[2]);
    dimDeltaZ->fontSize.connectFrom(&fieldFontSize);

    pDeltaDimensionSwitch = new SoSwitch();
    pDeltaDimensionSwitch->ref();
    pGlobalSeparator->addChild(pDeltaDimensionSwitch);

    pDeltaDimensionSwitch->addChild(dimDeltaX);
    pDeltaDimensionSwitch->addChild(dimDeltaY);
    pDeltaDimensionSwitch->addChild(dimDeltaZ);

    // setPart("textSep", nullptr) removes the label node; text="" still draws an empty box.
    dimDeltaX->setPart("textSep", nullptr);
    dimDeltaY->setPart("textSep", nullptr);
    dimDeltaZ->setPart("textSep", nullptr);

    pDeltaLabelSwitch = new SoSwitch();
    pDeltaLabelSwitch->ref();
    pDeltaLabelSwitch->whichChild.setValue(SO_SWITCH_NONE);

    static const int32_t leaderLineIdx[] = {0, 1, -1};

    for (int i = 0; i < 3; ++i) {
        pDeltaLabelTranslation[i] = new SoTransform();
        pDeltaLabelTranslation[i]->ref();

        pDeltaDragger[i] = new SoTranslate2Dragger();
        pDeltaDragger[i]->ref();
        pDeltaLabelTranslation[i]->translation.connectFrom(&pDeltaDragger[i]->translation);

        // point[0] is the anchor, updated in redrawAnnotation(); point[1] tracks the label.
        pDeltaLeaderCoords[i] = new SoCoordinate3();
        pDeltaLeaderCoords[i]->ref();
        pDeltaLeaderCoords[i]->point.setNum(1);
        pDeltaLeaderCoords[i]->point.set1Value(0, SbVec3f(0.0f, 0.0f, 0.0f));

        auto* leaderCat = new SoConcatenate(SoMFVec3f::getClassTypeId());
        leaderCat->input[0]->connectFrom(&pDeltaLeaderCoords[i]->point);
        leaderCat->input[1]->connectFrom(&pDeltaLabelTranslation[i]->translation);

        auto* leaderVerts = new SoVertexProperty();
        leaderVerts->vertex.connectFrom(leaderCat->output);

        auto* leaderLine = new SoIndexedLineSet();
        leaderLine->vertexProperty = leaderVerts;
        leaderLine->coordIndex.setValues(0, 3, leaderLineIdx);

        auto* leaderStyle = new SoDrawStyle();
        leaderStyle->lineWidth.setValue(1.0f);

        auto* leaderColor = new SoBaseColor();
        leaderColor->rgb.setValue(deltaAxisColors[i]);

        auto* leaderSep = new SoSeparator();
        leaderSep->addChild(leaderColor);
        leaderSep->addChild(leaderStyle);
        leaderSep->addChild(pDeltaLeaderCoords[i]);
        leaderSep->addChild(leaderLine);

        pDeltaLabel[i] = new Gui::SoFrameLabel();
        pDeltaLabel[i]->ref();
        pDeltaLabel[i]->justification = SoText2::CENTER;
        pDeltaLabel[i]->textColor.setValue(deltaAxisColors[i]);
        pDeltaLabel[i]->size.connectFrom(&fieldFontSize);
        pDeltaLabel[i]->backgroundColor.setValue(1.0f, 1.0f, 1.0f);  // matches DimensionLinear default

        auto* dragSep = new SoSeparator();
        dragSep->addChild(pDeltaDragger[i]);

        auto* labelPickStyle = new SoPickStyle();
        labelPickStyle->style = SoPickStyle::SHAPE_ON_TOP;

        auto* labelSep = new SoSeparator();
        labelSep->addChild(labelPickStyle);
        labelSep->addChild(dragSep);
        labelSep->addChild(pDeltaLabelTranslation[i]);
        labelSep->addChild(pDeltaLabel[i]);

        auto* combinedSep = new SoSeparator();
        combinedSep->addChild(leaderSep);
        combinedSep->addChild(labelSep);

        pDeltaLabelSwitch->addChild(combinedSep);
    }

    pRootSeparator->addChild(pDeltaLabelSwitch);

    pDeltaLabelSwitch->whichChild.connectFrom(&pDeltaDimensionSwitch->whichChild);

    // Make each label the drag handle via setPartAsPath("translator", ...).
    for (int i = 0; i < 3; ++i) {
        SoSearchAction sa;
        sa.setInterest(SoSearchAction::FIRST);
        sa.setSearchingAll(true);
        sa.setNode(pDeltaLabel[i]);
        sa.apply(pcRoot);
        SoPath* path = sa.getPath();
        if (path) {
            pDeltaDragger[i]->setPartAsPath("translator", path);
            pDeltaDragger[i]->setPart("translatorActive", nullptr);
            pDeltaDragger[i]->setPart("xAxisFeedback", nullptr);
            pDeltaDragger[i]->setPart("yAxisFeedback", nullptr);
        }
        else {
            Base::Console().warning(
                "ViewProviderMeasureDistance: delta label %d not found in scene graph; label will "
                "not be draggable.\n",
                i
            );
        }
    }
}

ViewProviderMeasureDistance::~ViewProviderMeasureDistance()
{
    pCoords->unref();
    pLines->unref();
    pDeltaDimensionSwitch->unref();
    pArrowSwitch->unref();
    for (int i = 0; i < 3; ++i) {
        pDeltaDragger[i]->unref();
        pDeltaLabelTranslation[i]->unref();
        pDeltaLabel[i]->unref();
        pDeltaLeaderCoords[i]->unref();
    }
    pDeltaLabelSwitch->unref();
}


//! repaint the annotation
void ViewProviderMeasureDistance::redrawAnnotation()
{
    if (!pcObject) {
        return;
    }

    auto prop1 = freecad_cast<App::PropertyVector*>(pcObject->getPropertyByName("Position1"));
    auto prop2 = freecad_cast<App::PropertyVector*>(pcObject->getPropertyByName("Position2"));

    if (!prop1 || !prop2) {
        return;
    }

    auto vec1 = prop1->getValue();
    auto vec2 = prop2->getValue();

    // Skip coincident points; measurement direction would be undefined.
    if ((vec2 - vec1).Length() < 1e-10) {
        return;
    }

    fieldPosition1.setValue(SbVec3f(vec1.x, vec1.y, vec1.z));
    fieldPosition2.setValue(SbVec3f(vec2.x, vec2.y, vec2.z));

    fieldDistance = (vec2 - vec1).Length();
    const float distance = fieldDistance.getValue();
    const float ta = distance / 2.0f;

    // Endpoints in local frame: ±half-distance along X axis.
    pCoords->point.set1Value(0, ta, 0.0f, 0.0f);
    pCoords->point.set1Value(1, -ta, 0.0f, 0.0f);

    auto* propDistance = dynamic_cast<App::PropertyDistance*>(pcObject->getPropertyByName("Distance"));
    if (propDistance) {
        setLabelValue(propDistance->getQuantityValue());
    }

    auto* propDistanceX = dynamic_cast<App::PropertyDistance*>(
        getMeasureObject()->getPropertyByName("DistanceX")
    );
    if (propDistanceX) {
        pDeltaLabel[0]->string.setValue(
            ("Δx: " + propDistanceX->getQuantityValue().getUserString()).c_str()
        );
    }

    auto* propDistanceY = dynamic_cast<App::PropertyDistance*>(
        getMeasureObject()->getPropertyByName("DistanceY")
    );
    if (propDistanceY) {
        pDeltaLabel[1]->string.setValue(
            ("Δy: " + propDistanceY->getQuantityValue().getUserString()).c_str()
        );
    }

    auto* propDistanceZ = dynamic_cast<App::PropertyDistance*>(
        getMeasureObject()->getPropertyByName("DistanceZ")
    );
    if (propDistanceZ) {
        pDeltaLabel[2]->string.setValue(
            ("Δz: " + propDistanceZ->getQuantityValue().getUserString()).c_str()
        );
    }

    SbMatrix matrix = getMatrix();
    pcTransform->setMatrix(matrix);

    // Anchor each leader at the midpoint of its delta segment, in local frame.
    {
        SbMatrix invMatrix = matrix.inverse();
        SbVec3f p1 = fieldPosition1.getValue();
        SbVec3f p2 = fieldPosition2.getValue();
        SbVec3f worldMids[3] = {
            SbVec3f((p1[0] + p2[0]) * 0.5f, p1[1], p1[2]),
            SbVec3f(p2[0], (p1[1] + p2[1]) * 0.5f, p1[2]),
            SbVec3f(p2[0], p2[1], (p1[2] + p2[2]) * 0.5f),
        };
        for (int i = 0; i < 3; ++i) {
            SbVec3f localMid;
            invMatrix.multVecMatrix(worldMids[i], localMid);
            pDeltaLeaderCoords[i]->point.set1Value(0, localMid);
        }
    }

    ViewProviderMeasureBase::redrawAnnotation();
    updateView();
}

void ViewProviderMeasureDistance::onChanged(const App::Property* prop)
{

    if (prop == &ShowDelta) {
        pDeltaDimensionSwitch->whichChild.setValue(
            ShowDelta.getValue() ? SO_SWITCH_ALL : SO_SWITCH_NONE
        );
        // pDeltaLabelSwitch follows via field connection.
    }
    else if (prop == &ShowArrows) {
        pArrowSwitch->whichChild.setValue(ShowArrows.getValue() ? SO_SWITCH_ALL : SO_SWITCH_NONE);
    }
    else if (prop == &ArrowSize) {
        const double clamped = std::clamp(ArrowSize.getValue(), 0.1, 10.0);
        if (clamped != ArrowSize.getValue()) {
            ArrowSize.setValue(clamped);
            return;
        }
        updateView();
    }
    else if (prop == &TextBackgroundColor) {
        auto bColor = TextBackgroundColor.getValue();
        static_cast<DimensionLinear*>(pDeltaDimensionSwitch->getChild(0))
            ->backgroundColor.setValue(bColor.r, bColor.g, bColor.b);
        static_cast<DimensionLinear*>(pDeltaDimensionSwitch->getChild(1))
            ->backgroundColor.setValue(bColor.r, bColor.g, bColor.b);
        static_cast<DimensionLinear*>(pDeltaDimensionSwitch->getChild(2))
            ->backgroundColor.setValue(bColor.r, bColor.g, bColor.b);
        for (int i = 0; i < 3; ++i) {
            pDeltaLabel[i]->backgroundColor.setValue(bColor.r, bColor.g, bColor.b);
        }
    }

    ViewProviderMeasureBase::onChanged(prop);
}


void ViewProviderMeasureDistance::arrowSizeCallback(void* data, SoAction* action)
{
    if (!action->isOfType(SoGLRenderAction::getClassTypeId())) {
        return;
    }
    auto* vp = static_cast<ViewProviderMeasureDistance*>(data);
    vp->drawArrowheads(action->getState());
}

void ViewProviderMeasureDistance::drawArrowheads(SoState* state)
{
    if (!pcObject) {
        return;
    }
    SbVec3f p1v = fieldPosition1.getValue();
    SbVec3f p2v = fieldPosition2.getValue();
    SbVec3f diff = p2v - p1v;
    if (diff.length() < 1e-10f) {
        return;
    }

    // world units per pixel at focal distance, same as SoDatumLabel
    const SbViewVolume& vv = SoViewVolumeElement::get(state);
    const SbViewportRegion& vpr = SoViewportRegionElement::get(state);
    SbVec2s vpSize = vpr.getViewportSizePixels();
    float focal = SoFocalDistanceElement::get(state);
    SbVec3f focal_pt = vv.getSightPoint(focal);
    float scalePx = vv.getWorldToScreenScale(focal_pt, 1.0F) / float(vpSize[0]);

    const float arrowH = scalePx * 14.0f * static_cast<float>(ArrowSize.getValue());
    const float halfW = arrowH * 0.4f;

    SbVec3f dir = diff;
    dir.normalize();

    SbVec3f projDir = vv.getProjectionDirection();
    SbVec3f camUp = vv.getViewUp();
    SbVec3f camRight = projDir.cross(camUp);
    camRight.normalize();
    camUp.normalize();

    float sdx = dir.dot(camRight);
    float sdy = dir.dot(camUp);
    SbVec3f perp = camRight * (-sdy) + camUp * sdx;
    if (perp.length() < 1e-6f) {
        perp = camRight;
    }
    perp.normalize();

    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
    glDisable(GL_LIGHTING);
    glColor3f(1.0f, 1.0f, 1.0f);

    auto drawArrow = [&](const SbVec3f& tip, const SbVec3f& awayDir) {
        SbVec3f base = tip + awayDir * arrowH;
        SbVec3f left = base - perp * halfW;
        SbVec3f right = base + perp * halfW;
        glBegin(GL_TRIANGLES);
        glVertex3f(tip[0], tip[1], tip[2]);
        glVertex3f(left[0], left[1], left[2]);
        glVertex3f(right[0], right[1], right[2]);
        glEnd();
    };

    drawArrow(p2v, -dir);
    drawArrow(p1v, dir);

    glPopAttrib();
}


void ViewProviderMeasureDistance::onLabelMoved()
{}


void ViewProviderMeasureDistance::finishRestoring()
{
    ViewProviderMeasureBase::finishRestoring();
    initDeltaLabelPositions();
}


void ViewProviderMeasureDistance::positionAnno(const Measure::MeasureBase* measureObject)
{
    (void)measureObject;

    if (!pcObject) {
        return;
    }

    auto prop1 = freecad_cast<App::PropertyVector*>(pcObject->getPropertyByName("Position1"));
    auto prop2 = freecad_cast<App::PropertyVector*>(pcObject->getPropertyByName("Position2"));
    if (!prop1 || !prop2) {
        return;
    }

    auto vec1 = prop1->getValue();
    auto vec2 = prop2->getValue();
    Base::Vector3d diff = vec2 - vec1;

    if (diff.Length() < 1e-10) {
        return;
    }

    Base::Vector3d midpoint = (vec1 + vec2) / 2.0;
    Base::Vector3d localXAxis = diff.Normalized();
    Base::Vector3d localYAxis = getTextDirection(localXAxis);

    float bboxExtent = static_cast<float>(
        std::max({std::abs(diff.x), std::abs(diff.y), std::abs(diff.z)})
    );
    float offset = std::max(bboxExtent * 0.7f, 0.15f * getViewScale());
    Base::Vector3d textPos = midpoint + localYAxis * offset;
    setLabelTranslation(SbVec3f(textPos.x, textPos.y, textPos.z));
    initDeltaLabelPositions();
    updateView();
}


void ViewProviderMeasureDistance::initDeltaLabelPositions()
{
    if (!pcObject) {
        return;
    }

    auto prop1 = freecad_cast<App::PropertyVector*>(pcObject->getPropertyByName("Position1"));
    auto prop2 = freecad_cast<App::PropertyVector*>(pcObject->getPropertyByName("Position2"));
    if (!prop1 || !prop2) {
        return;
    }

    auto vec1 = prop1->getValue();
    auto vec2 = prop2->getValue();
    if ((vec2 - vec1).Length() < 1e-10) {
        return;
    }

    SbMatrix invMatrix = getMatrix().inverse();
    float x1 = static_cast<float>(vec1.x);
    float y1 = static_cast<float>(vec1.y);
    float z1 = static_cast<float>(vec1.z);
    float x2 = static_cast<float>(vec2.x);
    float y2 = static_cast<float>(vec2.y);
    float z2 = static_cast<float>(vec2.z);

    SbVec3f worldMids[3] = {
        SbVec3f((x1 + x2) * 0.5f, y1, z1),
        SbVec3f(x2, (y1 + y2) * 0.5f, z1),
        SbVec3f(x2, y2, (z1 + z2) * 0.5f),
    };

    Base::Vector3d diff = vec2 - vec1;
    float bboxExtent = static_cast<float>(
        std::max({std::abs(diff.x), std::abs(diff.y), std::abs(diff.z)})
    );
    float offset = std::max(bboxExtent * 0.5f, 0.1f * getViewScale());

    for (int i = 0; i < 3; ++i) {
        SbVec3f localMid;
        invMatrix.multVecMatrix(worldMids[i], localMid);
        localMid[1] += offset;  // perpendicular offset in local Y
        pDeltaDragger[i]->translation.setValue(localMid);
    }
}
