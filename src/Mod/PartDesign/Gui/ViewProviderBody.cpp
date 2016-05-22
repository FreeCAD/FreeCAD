/***************************************************************************
 *   Copyright (c) 2011 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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
# include <boost/bind.hpp>
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/actions/SoGetBoundingBoxAction.h>
# include <Precision.hxx>
#endif

#include <Base/Console.h>
#include <App/Part.h>
#include <App/Origin.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/Application.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Gui/ViewProviderOrigin.h>
#include <Gui/ViewProviderOriginFeature.h>

#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/FeatureSketchBased.h>
#include <Mod/PartDesign/App/FeatureMultiTransform.h>
#include <Mod/PartDesign/App/DatumLine.h>
#include <Mod/PartDesign/App/DatumPlane.h>
#include <Mod/PartDesign/App/DatumCS.h>

#include "ViewProviderDatum.h"
#include "Utils.h"

#include "ViewProviderBody.h"
#include "ViewProvider.h"
#include <Gui/MDIView.h>

using namespace PartDesignGui;


PROPERTY_SOURCE(PartDesignGui::ViewProviderBody,PartGui::ViewProviderBodyBase)

ViewProviderBody::ViewProviderBody()
{
    sPixmap = "PartDesign_Body_Tree.svg";
}

ViewProviderBody::~ViewProviderBody()
{
}

void ViewProviderBody::attach(App::DocumentObject *pcFeat)
{
    // call parent attach method
    PartGui::ViewProviderBodyBase::attach(pcFeat);
}

bool ViewProviderBody::doubleClicked(void)
{
    //first, check if the body is already active.
    App::DocumentObject* activeBody = nullptr;
    Gui::MDIView* activeView = this->getActiveView();
    if ( activeView ) {
        activeBody = activeView->getActiveObject<App::DocumentObject*> (PDBODYKEY);
    }

    if (activeBody == this->getObject()){
        //deactivating...
    } else {
        //activating...
        Gui::Command::assureWorkbench("PartDesignWorkbench");
    }

    PartGui::ViewProviderBodyBase::doubleClicked(); // <-- active body switching happens there
    return true;
}

std::vector<App::DocumentObject*> ViewProviderBody::claimChildren(void)const
{
    PartDesign::Body* body= static_cast<PartDesign::Body*> ( getObject () );

    std::vector<App::DocumentObject*> result = ViewProviderBodyBase::claimChildren();

    if (body->BaseFeature.getValue()) { // Claim for the base feature: insert after Origin, but before all other features.
        size_t posToInsert = result.size() > 0 ? 1 : 0;
        result.insert(result.begin()+posToInsert, body->BaseFeature.getValue());
    }

    return result;
}


std::vector<App::DocumentObject*> ViewProviderBody::claimChildren3D(void)const
{
    PartDesign::Body* body = static_cast<PartDesign::Body*>(getObject());

    std::vector<App::DocumentObject*> rv = PartGui::ViewProviderBodyBase::claimChildren3D();

    if ( body->BaseFeature.getValue() ) { // Add Base Feature
        rv.push_back (body->BaseFeature.getValue());
    }

    return rv;
}

// TODO To be deleted (2015-09-08, Fat-Zer)
//void ViewProviderBody::updateTree()
//{
//    if (ActiveGuiDoc == NULL) return;
//
//    // Highlight active body and all its features
//    //Base::Console().Error("ViewProviderBody::updateTree()\n");
//    PartDesign::Body* body = static_cast<PartDesign::Body*>(getObject());
//    bool active = body->IsActive.getValue();
//    //Base::Console().Error("Body is %s\n", active ? "active" : "inactive");
//    ActiveGuiDoc->signalHighlightObject(*this, Gui::Blue, active);
//    std::vector<App::DocumentObject*> features = body->Model.getValues();
//    bool highlight = true;
//    App::DocumentObject* tip = body->Tip.getValue();
//    for (std::vector<App::DocumentObject*>::const_iterator f = features.begin(); f != features.end(); f++) {
//        //Base::Console().Error("Highlighting %s: %s\n", (*f)->getNameInDocument(), highlight ? "true" : "false");
//        Gui::ViewProviderDocumentObject* vp = dynamic_cast<Gui::ViewProviderDocumentObject*>(Gui::Application::Instance->getViewProvider(*f));
//        if (vp != NULL)
//            ActiveGuiDoc->signalHighlightObject(*vp, Gui::LightBlue, active ? highlight : false);
//        if (highlight && (tip == *f))
//            highlight = false;
//    }
//}

void ViewProviderBody::updateData(const App::Property* prop)
{
    PartDesign::Body* body = static_cast<PartDesign::Body*>(getObject());

    if (prop == &body->Model || prop == &body->BaseFeature) {
        // update sizes of origins and datums
        updateOriginDatumSize ();
        //ensure all model features are in visual body mode
        setVisualBodyMode(true);
    } 

    PartGui::ViewProviderBodyBase::updateData(prop);
}



void ViewProviderBody::updateOriginDatumSize () {
    PartDesign::Body *body = static_cast<PartDesign::Body *> ( getObject() );
    
    // Use different bounding boxes for datums and for origins:
    Gui::Document* gdoc = Gui::Application::Instance->getDocument(getObject()->getDocument());
    if(!gdoc) 
        return;
    
    Gui::MDIView* view = gdoc->getViewOfViewProvider(this);
    if(!view)
        return;
    
    Gui::View3DInventorViewer* viewer = static_cast<Gui::View3DInventor*>(view)->getViewer();
    SoGetBoundingBoxAction bboxAction(viewer->getSoRenderManager()->getViewportRegion());

    const auto & model = body->getFullModel ();

    // BBox for Datums is calculated from all visible objects but treating datums as their basepoints only
    SbBox3f bboxDatums = ViewProviderDatum::getRelevantBoundBox ( bboxAction, model );
    // BBox for origin should take into account datums size also
    SbBox3f bboxOrigins = bboxDatums;

    for(App::DocumentObject* obj : model) {
        if ( obj->isDerivedFrom ( Part::Datum::getClassTypeId () ) ) {
            ViewProvider *vp = Gui::Application::Instance->getViewProvider(obj);
            if (!vp) { continue; }

            ViewProviderDatum *vpDatum = static_cast <ViewProviderDatum *> (vp) ;

            vpDatum->setExtents ( bboxDatums );

            bboxAction.apply ( vp->getRoot () );
            bboxOrigins.extendBy ( bboxAction.getBoundingBox () );
        }
    }

    // get the bounding box values
    SbVec3f max = bboxOrigins.getMax();
    SbVec3f min = bboxOrigins.getMin();

    // obtain an Origin and it's ViewProvider
    App::Origin* origin = 0;
    Gui::ViewProviderOrigin* vpOrigin = 0;
    try {
        origin = body->getOrigin ();
        assert (origin);

        Gui::ViewProvider *vp = Gui::Application::Instance->getViewProvider(origin);
        if (!vp) {
            throw Base::Exception ("No view provider linked to the Origin");
        }
        assert ( vp->isDerivedFrom ( Gui::ViewProviderOrigin::getClassTypeId () ) );
        vpOrigin = static_cast <Gui::ViewProviderOrigin *> ( vp );
    } catch (const Base::Exception &ex) {
        Base::Console().Error ("%s\n", ex.what() );
        return;
    }

    // calculate the desired origin size
    Base::Vector3d size;

    for (uint_fast8_t i=0; i<3; i++) {
        size[i] = std::max ( fabs ( max[i] ), fabs ( min[i] ) );
        if (size[i] < Precision::Confusion() ) {
            size[i] = Gui::ViewProviderOrigin::defaultSize();
        }
    }

    vpOrigin->Size.setValue ( size*1.2 );
}

void ViewProviderBody::onChanged(const App::Property* prop) {
        
    if(prop != &DisplayModeBody) {
        unifyVisualProperty(prop);
    }
    
    PartGui::ViewProviderBodyBase::onChanged(prop);
}


void ViewProviderBody::unifyVisualProperty(const App::Property* prop) {

    if(prop == &Visibility || 
       prop == &Selectable ||
       prop == &DisplayModeBody)
        return;
                
    Gui::Document *gdoc = Gui::Application::Instance->getDocument ( pcObject->getDocument() ) ;
       
    PartDesign::Body *body = static_cast<PartDesign::Body *> ( getObject() );
    auto features = body->Model.getValues();
    for(auto feature : features) {
        
        if(!feature->isDerivedFrom(PartDesign::Feature::getClassTypeId()))
            continue;
        
        //copy over the properties data
        auto p = gdoc->getViewProvider(feature)->getPropertyByName(prop->getName());
        p->Paste(*prop);
    }
}

void ViewProviderBody::setVisualBodyMode(bool bodymode) {

    Gui::Document *gdoc = Gui::Application::Instance->getDocument ( pcObject->getDocument() ) ;
       
    PartDesign::Body *body = static_cast<PartDesign::Body *> ( getObject() );
    auto features = body->Model.getValues();
    for(auto feature : features) {
        
        if(!feature->isDerivedFrom(PartDesign::Feature::getClassTypeId()))
            continue;
        
        static_cast<PartDesignGui::ViewProvider*>(gdoc->getViewProvider(feature))->setBodyMode(bodymode);
    }
}

