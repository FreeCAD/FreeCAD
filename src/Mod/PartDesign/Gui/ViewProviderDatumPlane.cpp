/***************************************************************************
 *   Copyright (c) 2013 Jan Rheinländer                                    *
 *                                   <jrheinlaender@users.sourceforge.net> *
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
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoCoordinate3.h>
# include <Inventor/nodes/SoSwitch.h>
# include <Inventor/nodes/SoTransform.h>
#endif

#include <Gui/SoAxisCrossKit.h>
#include <Gui/SoFCBoundingBox.h>
#include <Mod/Part/Gui/SoBrepEdgeSet.h>
#include <Mod/Part/Gui/SoBrepFaceSet.h>
#include <Mod/PartDesign/App/DatumPlane.h>

#include "ViewProviderDatumPlane.h"

using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderDatumPlane,PartDesignGui::ViewProviderDatum)

ViewProviderDatumPlane::ViewProviderDatumPlane()
{
    sPixmap = "PartDesign_Plane.svg";

    pCoords = new SoCoordinate3();
    pCoords->ref ();

    // Hide the X and Y axes, we're only interested in showing the normal direction.
    auto axisKit = new Gui::SoAxisCrossKit;
    axisKit->set("xAxis.appearance.drawStyle", "style INVISIBLE");
    axisKit->set("xHead.appearance.drawStyle", "style INVISIBLE");
    axisKit->set("yAxis.appearance.drawStyle", "style INVISIBLE");
    axisKit->set("yHead.appearance.drawStyle", "style INVISIBLE");
    axisKit->set("zAxis.appearance.drawStyle", "lineWidth 2");

    auto coords = new SoCoordinate3;
    coords->point.set1Value(0, SbVec3f(0,0,0));
    coords->point.set1Value(1, SbVec3f(10,0,0));
    axisKit->setPart("zAxis.coordinate3", coords);
    axisKit->set("zHead.transform", "translation 0 0 15");
    axisKit->set("zHead.transform", "scaleFactor 0.5 1.5 0.5");

    auto axisCross = new Gui::SoShapeScale;
    axisCross->setPart("shape", axisKit);
    axisCross->scaleFactor = 1.0F;

    pTransform = new SoTransform;
    auto axisGroup = new Gui::SoSkipBoundingGroup;
    axisGroup->addChild(pTransform);
    axisGroup->addChild(axisCross);

    pArrowSwitch = new SoSwitch;
    pArrowSwitch->ref();

    pArrowSwitch->addChild(axisGroup);
    pArrowSwitch->whichChild = -1;
}

ViewProviderDatumPlane::~ViewProviderDatumPlane()
{
    pCoords->unref ();
}

void ViewProviderDatumPlane::attach ( App::DocumentObject *obj ) {

    ViewProviderDatum::attach ( obj );

    ViewProviderDatum::setExtents ( defaultBoundBox () );
    getShapeRoot ()->addChild(pCoords);

    PartGui::SoBrepEdgeSet* lineSet = new PartGui::SoBrepEdgeSet();
    lineSet->coordIndex.setNum(6);
    lineSet->coordIndex.set1Value(0, 0);
    lineSet->coordIndex.set1Value(1, 1);
    lineSet->coordIndex.set1Value(2, 2);
    lineSet->coordIndex.set1Value(3, 3);
    lineSet->coordIndex.set1Value(4, 0);
    lineSet->coordIndex.set1Value(5, SO_END_LINE_INDEX);
    getShapeRoot ()->addChild(lineSet);

    PartGui::SoBrepFaceSet *faceSet = new PartGui::SoBrepFaceSet();
    // SoBrepFaceSet supports only triangles (otherwise we receive incorrect highlighting)
    faceSet->partIndex.set1Value(0, 2); // One face, two triangles
    faceSet->coordIndex.setNum(8);
    // first triangle
    faceSet->coordIndex.set1Value(0, 0);
    faceSet->coordIndex.set1Value(1, 1);
    faceSet->coordIndex.set1Value(2, 2);
    faceSet->coordIndex.set1Value(3, SO_END_FACE_INDEX);
    // second triangle
    faceSet->coordIndex.set1Value(4, 2);
    faceSet->coordIndex.set1Value(5, 3);
    faceSet->coordIndex.set1Value(6, 0);
    faceSet->coordIndex.set1Value(7, SO_END_FACE_INDEX);

    getShapeRoot ()->addChild(faceSet);

    getShapeRoot ()->addChild(pArrowSwitch);
}

void ViewProviderDatumPlane::updateData(const App::Property* prop)
{
    // Gets called whenever a property of the attached object changes
    if (strcmp(prop->getName(),"Placement") == 0) {
        updateExtents ();
    }
    else if (strcmp(prop->getName(),"Length") == 0 ||
             strcmp(prop->getName(),"Width") == 0) {
        PartDesign::Plane* pcDatum = static_cast<PartDesign::Plane*>(this->getObject());
        if (pcDatum->ResizeMode.getValue() != 0)
            setExtents(pcDatum->Length.getValue(), pcDatum->Width.getValue());
    }

    ViewProviderDatum::updateData(prop);
}


void ViewProviderDatumPlane::setExtents (Base::BoundBox3d bbox) {
    PartDesign::Plane* pcDatum = static_cast<PartDesign::Plane*>(this->getObject());
    if (pcDatum->ResizeMode.getValue() != 0) {
        setExtents(pcDatum->Length.getValue(), pcDatum->Width.getValue());
        return;
    }

    Base::Placement plm = pcDatum->Placement.getValue ().inverse ();

    // Transform the box to the line's coordinates, the result line will be larger than the bbox
    bbox = bbox.Transformed ( plm.toMatrix() );
    // Add origin of the plane to the box if it's not
    bbox.Add ( Base::Vector3d (0, 0, 0) );

    double margin = sqrt(bbox.LengthX ()*bbox.LengthY ()) * marginFactor ();

    pcDatum->Length.setValue(bbox.LengthX() + 2*margin);
    pcDatum->Width.setValue(bbox.LengthY() + 2*margin);

    // Change the coordinates of the line
    pCoords->point.setNum (4);
    pCoords->point.set1Value(0, bbox.MaxX + margin, bbox.MaxY + margin, 0 );
    pCoords->point.set1Value(1, bbox.MinX - margin, bbox.MaxY + margin, 0 );
    pCoords->point.set1Value(2, bbox.MinX - margin, bbox.MinY - margin, 0 );
    pCoords->point.set1Value(3, bbox.MaxX + margin, bbox.MinY - margin, 0 );

    // Place the normal vector in the center.
    pTransform->translation.setValue((bbox.MaxX + bbox.MinX) / 2,
            (bbox.MaxY + bbox.MinY) / 2, 0);
}

void ViewProviderDatumPlane::setExtents(double l, double w)
{
    // Change the coordinates of the line
    pCoords->point.setNum (4);
    pCoords->point.set1Value(0, l/2, w/2, 0);
    pCoords->point.set1Value(1, -l/2, w/2, 0);
    pCoords->point.set1Value(2, -l/2, -w/2, 0);
    pCoords->point.set1Value(3, l/2, -w/2, 0);

    // Place the normal vector in the center.
    pTransform->translation.setValue(0, 0, 0);
}

bool ViewProviderDatumPlane::setEdit(int ModNum)
{
    // Show the normal vector.
    pArrowSwitch->whichChild = 0;
    return ViewProviderDatum::setEdit(ModNum);
}

void ViewProviderDatumPlane::unsetEdit(int ModNum)
{
    // Hide the normal vector.
    pArrowSwitch->whichChild = -1;
    ViewProviderDatum::unsetEdit(ModNum);
}
