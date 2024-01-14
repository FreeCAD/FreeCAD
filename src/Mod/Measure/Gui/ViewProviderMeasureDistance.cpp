/***************************************************************************
 *   Copyright (c) 2023 David Friedli <david[at]friedli-be.ch>             *
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
# include <sstream>
# include <QApplication>
# include <Inventor/engines/SoCalculator.h>
# include <Inventor/engines/SoConcatenate.h>
# include <Inventor/nodes/SoAnnotation.h>
# include <Inventor/nodes/SoBaseColor.h>
# include <Inventor/nodes/SoCoordinate3.h>
# include <Inventor/nodes/SoDrawStyle.h>
# include <Inventor/nodes/SoFontStyle.h>
# include <Inventor/nodes/SoIndexedLineSet.h>
# include <Inventor/nodes/SoMarkerSet.h>
# include <Inventor/nodes/SoPickStyle.h>
# include <Inventor/nodes/SoText2.h>
# include <Inventor/nodes/SoTranslation.h>
#endif

#include <Gui/Inventor/MarkerBitmaps.h>

#include <App/Document.h>
#include <App/MeasureDistance.h>
#include <Base/Console.h>
#include <Base/Quantity.h>
#include "Mod/Measure/App/MeasureDistance.h"
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


SbMatrix ViewProviderMeasureDistance::getMatrix() {
    auto object = dynamic_cast<Measure::MeasureDistance*>(getObject());
    auto vec1 = object->Position1.getValue();
    auto vec2 = object->Position2.getValue();

    const double tolerance(10.0e-6);
    SbVec3f origin = toSbVec3f((vec2 + vec1) / 2);
    Base::Vector3d localXAxis = (vec2 - vec1).Normalize();
    Base::Vector3d localYAxis = getTextDirection(localXAxis, tolerance).Normalize();

    // X and Y axis have to be 90° to eachother
    assert(fabs(localYAxis.Dot(localXAxis)) < tolerance);
    Base::Vector3d localZAxis = localYAxis.Cross(localXAxis).Normalize();

    SbMatrix matrix = SbMatrix(
        localXAxis.x, localXAxis.y, localXAxis.z, 0,
        localYAxis.x, localYAxis.y, localYAxis.z ,0,
        localZAxis.x, localZAxis.y, localZAxis.z, 0,
        // 0,0,0,1
        origin[0], origin[1], origin[2], 1
    );

    return matrix;
}


//! calculate a good direction from the elements being measured to the annotation text based on the layout
//! of the elements and its relationship with the cardinal axes and the view direction.  elementDirection
//! is expected to be a normalized vector.
//! an example of an elementDirection would be the vector from the start of a line to the end.
Base::Vector3d ViewProviderMeasureDistance::getTextDirection(Base::Vector3d elementDirection, double tolerance)
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
    sPixmap = "umf-measurement";

    // vert indexes used to create the annotation lines
    const size_t lineCount(3);
    static const int32_t lines[lineCount] =
    {
        2,3,-1 // dimension line
    };

    const size_t lineCountSecondary(9);
    static const int32_t linesSecondary[lineCountSecondary] = {
        0,2,-1, // extension line 1
        1,3,-1, // extension line 2
        2,4,-1  // label helper line
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

    pLines  = new SoIndexedLineSet();
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
    points->markerIndex = Gui::Inventor::MarkerBitmaps::getMarkerIndex("CROSS",
            ViewParams::instance()->getMarkerSize());
    points->numPoints=2;
    pLineSeparator->addChild(points);
}

ViewProviderMeasureDistance::~ViewProviderMeasureDistance()
{
    pCoords->unref();
    pLines->unref();
}


Measure::MeasureDistance* ViewProviderMeasureDistance::getMeasureDistance()
{
    Measure::MeasureDistance* feature = dynamic_cast<Measure::MeasureDistance*>(pcObject);
    if (!feature) {
        throw Base::RuntimeError("Feature not found for ViewProviderMeasureDistance");
    }
    return feature;
}

//! repaint the annotation
void ViewProviderMeasureDistance::redrawAnnotation()
{
    auto object = dynamic_cast<Measure::MeasureDistance*>(getObject());
    auto vec1 = object->Position1.getValue();
    auto vec2 = object->Position2.getValue();

    // Set the distance
    fieldDistance = (vec2 - vec1).Length();

    setLabelValue(object->Distance.getQuantityValue().getUserString());

    // Set matrix
    SbMatrix matrix = getMatrix();
    pcTransform->setMatrix(matrix);

    ViewProviderMeasureBase::redrawAnnotation();
    updateView();
}


void ViewProviderMeasureDistance::onGuiInit(const Measure::MeasureBase* measureObject) {
    (void)measureObject;
    setLabelTranslation(SbVec3f(0, 10, 0));
}
