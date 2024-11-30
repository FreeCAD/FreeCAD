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

#include "PreCompiled.h"

#ifndef _PreComp_
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
#include <Inventor/nodes/SoMarkerSet.h>
#include <Inventor/nodes/SoPickStyle.h>
#include <Inventor/nodes/SoText2.h>
#include <Inventor/nodes/SoTranslation.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoCone.h>
#include <Inventor/nodes/SoResetTransform.h>
#include <Inventor/nodes/SoNodes.h>
#endif

#include <Gui/Inventor/MarkerBitmaps.h>

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
    SO_NODE_ADD_FIELD(showArrows, (false));  // display dimension arrows
    SO_NODE_ADD_FIELD(fontSize, (12.0));     // size of the dimension font
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

    // dimension arrows
    float dimLength = (point2.getValue() - point1.getValue()).length();
    float coneHeight = dimLength * 0.06;
    float coneRadius = coneHeight * 0.5;

    SoComposeVec3f* vec = new SoComposeVec3f;
    vec->x.connectFrom(&length);
    vec->y.setValue(0.0);
    vec->z.setValue(0.0);

    // NOTE: showArrows is only respected at setup stage and cannot be changed later
    if (showArrows.getValue()) {
        SoCone* cone = new SoCone();
        cone->bottomRadius.setValue(coneRadius);
        cone->height.setValue(coneHeight);

        char lStr[100];
        char rStr[100];
        snprintf(lStr, sizeof(lStr), "translation %.6f 0.0 0.0", coneHeight * 0.5);
        snprintf(rStr, sizeof(rStr), "translation 0.0 -%.6f 0.0", coneHeight * 0.5);

        setPart("leftArrow.shape", cone);
        set("leftArrow.transform", "rotation 0.0 0.0 1.0 1.5707963");
        set("leftArrow.transform", lStr);
        setPart("rightArrow.shape", cone);
        set("rightArrow.transform", "rotation 0.0 0.0 -1.0 1.5707963");  // no constant for PI.
        // have use local here to do the offset because the main is wired up to length of dimension.
        set("rightArrow.localTransform", rStr);

        SoTransform* transform = static_cast<SoTransform*>(getPart("rightArrow.transform", false));
        if (!transform) {
            return;  // what to do here?
        }
        transform->translation.connectFrom(&vec->vector);

        setPart("leftArrow.material", material);
        setPart("rightArrow.material", material);
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
    textVecCalc->B.set1Value(0, 0.0, 0.250, 0.0);
    textVecCalc->expression.set1Value(0, "oA = (A / 2) + B");

    SoTransform* textTransform = new SoTransform();
    textTransform->translation.connectFrom(&textVecCalc->oA);
    textSep->addChild(textTransform);

    SoFont* fontNode = new SoFont();
    fontNode->name.setValue("Helvetica : Bold");
    fontNode->size.connectFrom(&fontSize);
    textSep->addChild(fontNode);

    auto textNode = new SoFrameLabel();
    textNode->justification = SoText2::CENTER;
    textNode->string.connectFrom(&text);
    textNode->textColor.connectFrom(&dColor);
    textNode->backgroundColor.connectFrom(&backgroundColor);
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

    auto prop1 =
        Base::freecad_dynamic_cast<App::PropertyVector>(pcObject->getPropertyByName("Position1"));
    auto prop2 =
        Base::freecad_dynamic_cast<App::PropertyVector>(pcObject->getPropertyByName("Position2"));

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
    Base::Vector3d localZAxis = localYAxis.Cross(localXAxis).Normalize();

    SbMatrix matrix = SbMatrix(localXAxis.x,
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
                               1);

    return matrix;
}


//! calculate a good direction from the elements being measured to the annotation text based on the
//! layout of the elements and its relationship with the cardinal axes and the view direction.
//! elementDirection is expected to be a normalized vector. an example of an elementDirection would
//! be the vector from the start of a line to the end.
Base::Vector3d ViewProviderMeasureDistance::getTextDirection(Base::Vector3d elementDirection,
                                                             double tolerance)
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

    ADD_PROPERTY_TYPE(ShowDelta,
                      (false),
                      "Appearance",
                      App::Prop_None,
                      "Display the X, Y and Z components of the distance");

    // vert indexes used to create the annotation lines
    const size_t lineCount(3);
    static const int32_t lines[lineCount] = {
        2,
        3,
        -1  // dimension line
    };

    const size_t lineCountSecondary(9);
    static const int32_t linesSecondary[lineCountSecondary] = {
        0,
        2,
        -1,  // extension line 1
        1,
        3,
        -1,  // extension line 2
        2,
        4,
        -1  // label helper line
    };

    // Line Coordinates
    // 0-1 points on shape (dimension points)
    // 2-3 ends of extension lines/dimension line
    // 4 label position
    pCoords = new SoCoordinate3();
    pCoords->ref();

    auto engineCoords = new SoCalculator();
    engineCoords->a.connectFrom(&fieldDistance);
    engineCoords->A.connectFrom(&pLabelTranslation->translation);
    engineCoords->expression.setValue("ta=a/2; tb=A[1]; oA=vec3f(ta, 0, 0); oB=vec3f(-ta, 0, 0); "
                                      "oC=vec3f(ta, tb, 0); oD=vec3f(-ta, tb, 0)");

    auto engineCat = new SoConcatenate(SoMFVec3f::getClassTypeId());
    engineCat->input[0]->connectFrom(&engineCoords->oA);
    engineCat->input[1]->connectFrom(&engineCoords->oB);
    engineCat->input[2]->connectFrom(&engineCoords->oC);
    engineCat->input[3]->connectFrom(&engineCoords->oD);
    engineCat->input[4]->connectFrom(&pLabelTranslation->translation);

    pCoords->point.connectFrom(engineCat->output);
    pCoords->point.setNum(engineCat->output->getNumConnections());

    pLines = new SoIndexedLineSet();
    pLines->ref();
    pLines->coordIndex.setNum(lineCount);
    pLines->coordIndex.setValues(0, lineCount, lines);

    pLineSeparator->addChild(pCoords);
    pLineSeparator->addChild(pLines);


    // Secondary Lines
    auto lineSetSecondary = new SoIndexedLineSet();
    lineSetSecondary->coordIndex.setNum(lineCountSecondary);
    lineSetSecondary->coordIndex.setValues(0, lineCountSecondary, linesSecondary);

    pLineSeparatorSecondary->addChild(pCoords);
    pLineSeparatorSecondary->addChild(lineSetSecondary);

    auto points = new SoMarkerSet();
    points->markerIndex =
        Gui::Inventor::MarkerBitmaps::getMarkerIndex("CROSS",
                                                     ViewParams::instance()->getMarkerSize());
    points->numPoints = 2;
    pLineSeparator->addChild(points);


    // Delta Dimensions
    auto decomposedPosition1 = new SoDecomposeVec3f();
    decomposedPosition1->vector.connectFrom(&fieldPosition1);
    auto decomposedPosition2 = new SoDecomposeVec3f();
    decomposedPosition2->vector.connectFrom(&fieldPosition2);

    // Create intermediate points
    auto composeVecDelta1 = new SoComposeVec3f();
    composeVecDelta1->x.connectFrom(&decomposedPosition2->x);
    composeVecDelta1->y.connectFrom(&decomposedPosition1->y);
    composeVecDelta1->z.connectFrom(&decomposedPosition1->z);

    auto composeVecDelta2 = new SoComposeVec3f();
    composeVecDelta2->x.connectFrom(&decomposedPosition2->x);
    composeVecDelta2->y.connectFrom(&decomposedPosition2->y);
    composeVecDelta2->z.connectFrom(&decomposedPosition1->z);

    // Set axis colors
    SbColor colorX;
    SbColor colorY;
    SbColor colorZ;

    float t = 0.0f;
    colorX.setPackedValue(ViewParams::instance()->getAxisXColor(), t);
    colorY.setPackedValue(ViewParams::instance()->getAxisYColor(), t);
    colorZ.setPackedValue(ViewParams::instance()->getAxisZColor(), t);

    auto dimDeltaX = new MeasureGui::DimensionLinear();
    dimDeltaX->point1.connectFrom(&fieldPosition1);
    dimDeltaX->point2.connectFrom(&composeVecDelta1->vector);
    dimDeltaX->setupDimension();
    dimDeltaX->dColor.setValue(colorX);

    auto dimDeltaY = new MeasureGui::DimensionLinear();
    dimDeltaY->point1.connectFrom(&composeVecDelta1->vector);
    dimDeltaY->point2.connectFrom(&composeVecDelta2->vector);
    dimDeltaY->setupDimension();
    dimDeltaY->dColor.setValue(colorY);

    auto dimDeltaZ = new MeasureGui::DimensionLinear();
    dimDeltaZ->point2.connectFrom(&composeVecDelta2->vector);
    dimDeltaZ->point1.connectFrom(&fieldPosition2);
    dimDeltaZ->setupDimension();
    dimDeltaZ->dColor.setValue(colorZ);

    pDeltaDimensionSwitch = new SoSwitch();
    pDeltaDimensionSwitch->ref();
    pGlobalSeparator->addChild(pDeltaDimensionSwitch);

    pDeltaDimensionSwitch->addChild(dimDeltaX);
    pDeltaDimensionSwitch->addChild(dimDeltaY);
    pDeltaDimensionSwitch->addChild(dimDeltaZ);

    // This should already be touched in ViewProviderMeasureBase
    FontSize.touch();
}

ViewProviderMeasureDistance::~ViewProviderMeasureDistance()
{
    pCoords->unref();
    pLines->unref();
    pDeltaDimensionSwitch->unref();
}


//! repaint the annotation
void ViewProviderMeasureDistance::redrawAnnotation()
{
    if (!pcObject) {
        return;
    }

    auto prop1 =
        Base::freecad_dynamic_cast<App::PropertyVector>(pcObject->getPropertyByName("Position1"));
    auto prop2 =
        Base::freecad_dynamic_cast<App::PropertyVector>(pcObject->getPropertyByName("Position2"));

    if (!prop1 || !prop2) {
        return;
    }

    auto vec1 = prop1->getValue();
    auto vec2 = prop2->getValue();

    fieldPosition1.setValue(SbVec3f(vec1.x, vec1.y, vec1.z));
    fieldPosition2.setValue(SbVec3f(vec2.x, vec2.y, vec2.z));

    // Set the distance
    fieldDistance = (vec2 - vec1).Length();

    auto propDistance =
        dynamic_cast<App::PropertyDistance*>(pcObject->getPropertyByName("Distance"));
    setLabelValue(propDistance->getQuantityValue().getUserString());

    // Set delta distance
    auto propDistanceX =
        static_cast<App::PropertyDistance*>(getMeasureObject()->getPropertyByName("DistanceX"));
    static_cast<DimensionLinear*>(pDeltaDimensionSwitch->getChild(0))
        ->text.setValue("Δx: " + propDistanceX->getQuantityValue().getUserString().toUtf8());

    auto propDistanceY =
        static_cast<App::PropertyDistance*>(getMeasureObject()->getPropertyByName("DistanceY"));
    static_cast<DimensionLinear*>(pDeltaDimensionSwitch->getChild(1))
        ->text.setValue("Δy: " + propDistanceY->getQuantityValue().getUserString().toUtf8());

    auto propDistanceZ =
        static_cast<App::PropertyDistance*>(getMeasureObject()->getPropertyByName("DistanceZ"));
    static_cast<DimensionLinear*>(pDeltaDimensionSwitch->getChild(2))
        ->text.setValue("Δz: " + propDistanceZ->getQuantityValue().getUserString().toUtf8());

    // Set matrix
    SbMatrix matrix = getMatrix();
    pcTransform->setMatrix(matrix);

    ViewProviderMeasureBase::redrawAnnotation();
    updateView();
}

void ViewProviderMeasureDistance::onChanged(const App::Property* prop)
{

    if (prop == &ShowDelta) {
        pDeltaDimensionSwitch->whichChild.setValue(ShowDelta.getValue() ? SO_SWITCH_ALL
                                                                        : SO_SWITCH_NONE);
    }
    else if (prop == &FontSize) {
        static_cast<DimensionLinear*>(pDeltaDimensionSwitch->getChild(0))
            ->fontSize.setValue(FontSize.getValue());
        static_cast<DimensionLinear*>(pDeltaDimensionSwitch->getChild(1))
            ->fontSize.setValue(FontSize.getValue());
        static_cast<DimensionLinear*>(pDeltaDimensionSwitch->getChild(2))
            ->fontSize.setValue(FontSize.getValue());
    }
    else if (prop == &TextBackgroundColor) {
        auto bColor = TextBackgroundColor.getValue();
        static_cast<DimensionLinear*>(pDeltaDimensionSwitch->getChild(0))
            ->backgroundColor.setValue(bColor.r, bColor.g, bColor.g);
        static_cast<DimensionLinear*>(pDeltaDimensionSwitch->getChild(1))
            ->backgroundColor.setValue(bColor.r, bColor.g, bColor.g);
        static_cast<DimensionLinear*>(pDeltaDimensionSwitch->getChild(2))
            ->backgroundColor.setValue(bColor.r, bColor.g, bColor.g);
    }


    ViewProviderMeasureBase::onChanged(prop);
}


void ViewProviderMeasureDistance::positionAnno(const Measure::MeasureBase* measureObject)
{
    (void)measureObject;
    setLabelTranslation(SbVec3f(0, 0.1 * getViewScale(), 0));
}
