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
# include <Precision.hxx>
#endif

#include <Base/Tools.h>
#include <Gui/SoFCSelectionAction.h>
#include <Mod/Part/Gui/SoBrepFaceSet.h>
#include <Mod/Part/Gui/SoBrepEdgeSet.h>
#include <Mod/PartDesign/App/DatumPlane.h>

#include "ViewProviderDatumPlane.h"

using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderDatumPlane,PartDesignGui::ViewProviderDatum)

ViewProviderDatumPlane::ViewProviderDatumPlane()
{
    sPixmap = "PartDesign_Plane.svg";

    pCoords = new SoCoordinate3();
}

ViewProviderDatumPlane::~ViewProviderDatumPlane()
{
}

void ViewProviderDatumPlane::attach ( App::DocumentObject *obj ) {

    ViewProviderDatum::attach ( obj );

    ViewProviderDatum::setExtents ( defaultBoundBox () );
    getShapeRoot ()->addChild(pCoords);

    PartGui::SoBrepFaceSet *faceSet = new PartGui::SoBrepFaceSet();
    pFaceSet.reset(faceSet);
    
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
}

void ViewProviderDatumPlane::updateData(const App::Property* prop)
{
    if(!prop)
        return;
    auto name = prop->getName();
    if(name && !prop->testStatus(App::Property::User3)) {
        if(strcmp(name, "Placement")==0
                || strcmp(name, "Length")==0
                || strcmp(name, "Width")==0
                || strcmp(name, "ResizeMode")==0)
        {
            updateExtents();
        }
    }

    ViewProviderDatum::updateData(prop);
}

void ViewProviderDatumPlane::updateExtents() {
    auto pcDatum = Base::freecad_dynamic_cast<PartDesign::Plane>(getObject());
    if (pcDatum && pcDatum->ResizeMode.getValue() != 0)
        setExtents(pcDatum->Length.getValue(), pcDatum->Width.getValue());
    else
        ViewProviderDatum::updateExtents();
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

    double length = bbox.LengthX();
    if (length < pcDatum->MinimumLength.getValue()) {
        length = pcDatum->MinimumLength.getValue();
        double center = (bbox.MaxX - bbox.MinX) / 2.0 + bbox.MinX;
        bbox.MaxX = center + length/2.0; 
        bbox.MinX = center - length/2.0; 
    }
    if (length != pcDatum->Length.getValue()) {
        Base::ObjectStatusLocker<App::Property::Status, App::Property> guard(App::Property::User3, &pcDatum->Length);
        pcDatum->Length.setValue(length);
    }

    double width = bbox.LengthY();
    if (width < pcDatum->MinimumWidth.getValue()) {
        width = pcDatum->MinimumWidth.getValue();
        double center = (bbox.MaxY - bbox.MinY) / 2.0 + bbox.MinY;
        bbox.MaxY = center + width/2.0; 
        bbox.MinY = center - width/2.0; 
    }
    if (width != pcDatum->Width.getValue()) {
        Base::ObjectStatusLocker<App::Property::Status, App::Property> guard(App::Property::User3, &pcDatum->Width);
        pcDatum->Width.setValue(width);
    }

    double marginx = length * marginFactor ();
    double marginy = width * marginFactor ();

    // Change the coordinates of the line
    pCoords->point.setNum (4);
    pCoords->point.set1Value(0, bbox.MaxX + marginx, bbox.MaxY + marginy, 0 );
    pCoords->point.set1Value(1, bbox.MinX - marginx, bbox.MaxY + marginy, 0 );
    pCoords->point.set1Value(2, bbox.MinX - marginx, bbox.MinY - marginy, 0 );
    pCoords->point.set1Value(3, bbox.MaxX + marginx, bbox.MinY - marginy, 0 );

    if(pFaceSet) {
        Gui::SoUpdateVBOAction action;
        action.apply(this->pFaceSet);
    }
}

void ViewProviderDatumPlane::setExtents(double l, double w)
{
    // Change the coordinates of the line
    pCoords->point.setNum (4);
    pCoords->point.set1Value(0, l/2, w/2, 0);
    pCoords->point.set1Value(1, -l/2, w/2, 0);
    pCoords->point.set1Value(2, -l/2, -w/2, 0);
    pCoords->point.set1Value(3, l/2, -w/2, 0);

    if(pFaceSet) {
        Gui::SoUpdateVBOAction action;
        action.apply(this->pFaceSet);
    }

}
