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
#endif

#include <Mod/Part/Gui/SoBrepEdgeSet.h>
#include <Mod/PartDesign/App/DatumLine.h>

#include "ViewProviderDatumLine.h"

using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderDatumLine,PartDesignGui::ViewProviderDatum)

ViewProviderDatumLine::ViewProviderDatumLine() {
    sPixmap = "PartDesign_Line.svg";

    pCoords = new SoCoordinate3();
    pCoords->ref ();
}

ViewProviderDatumLine::~ViewProviderDatumLine() {
    pCoords->unref ();
}

void ViewProviderDatumLine::attach ( App::DocumentObject *obj ) {
    ViewProviderDatum::attach ( obj );

    PartGui::SoBrepEdgeSet* lineSet;

    ViewProviderDatum::setExtents ( defaultBoundBox () );
    getShapeRoot ()->addChild(pCoords);

    lineSet = new PartGui::SoBrepEdgeSet();
    lineSet->coordIndex.setNum(3);
    lineSet->coordIndex.set1Value(0, 0);
    lineSet->coordIndex.set1Value(1, 1);
    lineSet->coordIndex.set1Value(2, SO_END_LINE_INDEX);

    getShapeRoot ()->addChild(lineSet);
}

void ViewProviderDatumLine::updateData(const App::Property* prop)
{
    // Gets called whenever a property of the attached object changes
    if (strcmp(prop->getName(),"Placement") == 0) {
        updateExtents ();
    }
    else if (strcmp(prop->getName(),"Length") == 0) {
        PartDesign::Line* pcDatum = static_cast<PartDesign::Line*>(this->getObject());
        if (pcDatum->ResizeMode.getValue() != 0)
            setExtents(pcDatum->Length.getValue());
    }

    ViewProviderDatum::updateData(prop);
}


void ViewProviderDatumLine::setExtents (Base::BoundBox3d bbox) {
    PartDesign::Line* pcDatum = static_cast<PartDesign::Line*>(this->getObject());
    // set manual size
    if (pcDatum->ResizeMode.getValue() != 0) {
        setExtents(pcDatum->Length.getValue());
        return;
    }
    Base::Placement plm = pcDatum->Placement.getValue ().inverse ();

    // Transform the box to the line's coordinates, the result line will be larger than the bbox
    bbox = bbox.Transformed ( plm.toMatrix() );
    // Add origin of the line to the box if it's not
    bbox.Add ( Base::Vector3d (0, 0, 0) );

    double margin = bbox.LengthZ () * marginFactor ();

    // Display the line
    pCoords->point.setNum (2);
    pCoords->point.set1Value(0, 0, 0, bbox.MaxZ + margin );
    pCoords->point.set1Value(1, 0, 0, bbox.MinZ - margin );
}

void ViewProviderDatumLine::setExtents(double l)
{
    // Change the coordinates of the line
    pCoords->point.setNum (2);
    pCoords->point.set1Value(0, 0, 0, l/2 );
    pCoords->point.set1Value(1, 0, 0, -l/2 );
}
