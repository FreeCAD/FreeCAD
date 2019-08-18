/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger <stefantroeger@gmx.net>              *
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
# include <Inventor/nodes/SoText2.h>
# include <Inventor/nodes/SoCoordinate3.h>
# include <Inventor/nodes/SoDrawStyle.h>
# include <Inventor/nodes/SoFont.h>
# include <Inventor/nodes/SoMaterial.h>
# include <Inventor/nodes/SoRotation.h>
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoTranslation.h>
# include <Inventor/nodes/SoSwitch.h>
# include <Inventor/details/SoLineDetail.h>
#endif

#include <App/Application.h>
#include <Gui/Inventor/SoAutoZoomTranslation.h>
#include "TaskDatumParameters.h"
#include <Mod/Part/Gui/SoBrepEdgeSet.h>

#include "ViewProviderDatumCS.h"

using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderDatumCoordinateSystem,PartDesignGui::ViewProviderDatum)

const App::PropertyFloatConstraint::Constraints ZoomConstraint = {0.0,DBL_MAX,0.2};
const App::PropertyIntegerConstraint::Constraints FontConstraint = {1,INT_MAX,1};

ViewProviderDatumCoordinateSystem::ViewProviderDatumCoordinateSystem()
{
    Zoom.setConstraints(&ZoomConstraint);
    FontSize.setConstraints(&FontConstraint);

    auto hGrp = App::GetApplication().GetParameterGroupByPath(
             "User parameter:BaseApp/Preferences/Mod/PartDesign");
    auto fontSize = hGrp->GetInt("CoordinateSystemFontSize",10);
    auto zoom = hGrp->GetFloat("CoordinateSystemZoom",1.0);
    auto showLabel = hGrp->GetBool("CoordinateSystemShowLabel",false);

    ADD_PROPERTY_TYPE(FontSize, (fontSize), "Datum", App::Prop_None, "");
    ADD_PROPERTY_TYPE(Zoom, (zoom), "Datum", App::Prop_None, "");
    ADD_PROPERTY_TYPE(ShowLabel, (showLabel), "Datum", App::Prop_None, "");

    if(hGrp->GetBool("CoordinateSystemSelectOnTop",true))
        OnTopWhenSelected.setValue(1);

    sPixmap = "PartDesign_CoordinateSystem.svg";

    coord = new SoCoordinate3();
    coord->ref();
    font = new SoFont();
    font->size = FontSize.getValue();
    font->ref();
    axisLabelXTrans = new SoTranslation();
    axisLabelXTrans->ref();
    axisLabelXToYTrans = new SoTranslation();
    axisLabelXToYTrans->ref();
    axisLabelYToZTrans = new SoTranslation();
    axisLabelYToZTrans->ref();

    autoZoom = new Gui::SoAutoZoomTranslation;
    autoZoom->ref();

    labelSwitch = 0;
}

ViewProviderDatumCoordinateSystem::~ViewProviderDatumCoordinateSystem()
{
    coord->unref();
    font->unref();
    axisLabelXTrans->unref();
    axisLabelXToYTrans->unref();
    axisLabelYToZTrans->unref();
    if(labelSwitch)
        labelSwitch->unref();
    autoZoom->unref();
}

void ViewProviderDatumCoordinateSystem::attach ( App::DocumentObject *obj ) {
    ViewProviderDatum::attach ( obj );

    SoMaterial* material = new SoMaterial();
    material->diffuseColor.setNum(4);
    material->diffuseColor.set1Value(0, SbColor(0.f, 0.f, 0.f));
    material->diffuseColor.set1Value(1, SbColor(1.f, 0.f, 0.f));
    material->diffuseColor.set1Value(2, SbColor(0.f, 0.6f, 0.f));
    material->diffuseColor.set1Value(3, SbColor(0.f, 0.f, 1.f));
    SoMaterialBinding* binding = new SoMaterialBinding();
    binding->value = SoMaterialBinding::PER_FACE_INDEXED;

    autoZoom->scaleFactor.setValue(Zoom.getValue());
    getShapeRoot ()->addChild(autoZoom);
    getShapeRoot ()->addChild(binding);
    getShapeRoot ()->addChild(material);

    coord->point.setNum(4);

    ViewProviderDatum::setExtents ( defaultBoundBox () );

    getShapeRoot ()->addChild(coord);

    SoDrawStyle* style = new SoDrawStyle ();
    style->lineWidth = 1.5f;
    getShapeRoot ()->addChild(style);

    PartGui::SoBrepEdgeSet* lineSet = new PartGui::SoBrepEdgeSet();
    lineSet->coordIndex.setNum(9);
    // X
    lineSet->coordIndex.set1Value(0, 0);
    lineSet->coordIndex.set1Value(1, 1);
    lineSet->coordIndex.set1Value(2, SO_END_LINE_INDEX);
    // Y
    lineSet->coordIndex.set1Value(3, 0);
    lineSet->coordIndex.set1Value(4, 2);
    lineSet->coordIndex.set1Value(5, SO_END_LINE_INDEX);
    // Z
    lineSet->coordIndex.set1Value(6, 0);
    lineSet->coordIndex.set1Value(7, 3);
    lineSet->coordIndex.set1Value(8, SO_END_LINE_INDEX);

    lineSet->materialIndex.setNum(3);
    lineSet->materialIndex.set1Value(0,1);
    lineSet->materialIndex.set1Value(1,2);
    lineSet->materialIndex.set1Value(2,3);
    getShapeRoot ()->addChild(lineSet);

    setupLabels();
}

void ViewProviderDatumCoordinateSystem::setupLabels() {

    if(!ShowLabel.getValue()) {
        if(labelSwitch)
            labelSwitch->whichChild = -1;
        return;
    }else if(labelSwitch) {
        labelSwitch->whichChild = 0;
        return;
    }

    labelSwitch = new SoSwitch;
    labelSwitch->ref();

    getShapeRoot ()->addChild(labelSwitch);

    SoGroup *labelGroup = new SoGroup;
    labelSwitch->addChild(labelGroup);
    labelSwitch->whichChild = 0;

    labelGroup->addChild(font);

    // Transformation for axis labels are relative so no need in separators
    labelGroup->addChild(axisLabelXTrans);
    auto* t = new SoText2();
    t->string = "X";
    labelGroup->addChild(t);

    labelGroup->addChild(axisLabelXToYTrans);
    t = new SoText2();
    t->string = "Y";
    labelGroup->addChild(t);

    labelGroup->addChild(axisLabelYToZTrans);
    t = new SoText2();
    t->string = "Z";
    labelGroup->addChild(t);
}

void ViewProviderDatumCoordinateSystem::updateData(const App::Property* prop)
{
    if (strcmp(prop->getName(),"Placement") == 0) 
        updateExtents ();

    ViewProviderDatum::updateData(prop);
}

void ViewProviderDatumCoordinateSystem::onChanged(const App::Property *prop) {
    if(getObject()) {
        if(prop == &ShowLabel)
            setupLabels();
        else if(prop == &Zoom) {
            autoZoom->scaleFactor.setValue(Zoom.getValue());
            updateExtents ();
        } else if(prop == &FontSize) 
            font->size = FontSize.getValue();
    }
    ViewProviderDatum::onChanged(prop);
}

void ViewProviderDatumCoordinateSystem::setExtents (Base::BoundBox3d bbox) {
    // Axis length of the CS is 1/3 of maximum bbox dimension, any smarter sizing will make it only worse
    double axisLength;
    
    if(Zoom.getValue()) {
        axisLength = 6 * Zoom.getValue();
    }else{
        axisLength = std::max ( { bbox.LengthX (), bbox.LengthY(), bbox.LengthZ() } );
        axisLength *= (1 + marginFactor ()) / 3;
    }

    coord->point.set1Value ( 0, 0, 0, 0 );
    coord->point.set1Value ( 1, axisLength, 0, 0 );
    coord->point.set1Value ( 2, 0, axisLength, 0 );
    coord->point.set1Value ( 3, 0, 0, axisLength );

    double labelPos = axisLength;
    double labelOffset = 0;

    // offset 1 pixel
    axisLabelXTrans->translation.setValue ( SbVec3f( labelPos, labelOffset, 0) );
    axisLabelXToYTrans->translation.setValue ( SbVec3f( -labelPos + labelOffset, labelPos - labelOffset, 0) );
    axisLabelYToZTrans->translation.setValue ( SbVec3f( -labelOffset, -labelPos + labelOffset, labelPos) );
}

std::string ViewProviderDatumCoordinateSystem::getElement(const SoDetail* detail) const
{
    if (detail && detail->getTypeId() == SoLineDetail::getClassTypeId()) {
        const SoLineDetail* line_detail = static_cast<const SoLineDetail*>(detail);
        switch(line_detail->getLineIndex()) {
        case 0:
            return "X";
        case 1:
            return "Y";
        case 2:
            return "Z";
        }
    }

    return std::string();
}

SoDetail* ViewProviderDatumCoordinateSystem::getDetail(const char* subelement) const
{
    if (strcmp(subelement,"X")==0) {
         SoLineDetail* detail = new SoLineDetail();
         detail->setLineIndex(0);
         return detail;
    } else if (strcmp(subelement,"Y")==0) {
         SoLineDetail* detail = new SoLineDetail();
         detail->setLineIndex(1);
         return detail;
    } else if (strcmp(subelement,"Z")==0) {
         SoLineDetail* detail = new SoLineDetail();
         detail->setLineIndex(2);
         return detail;
    }
    return NULL;
}

