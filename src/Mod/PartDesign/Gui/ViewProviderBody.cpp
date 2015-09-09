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

using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderBody,PartGui::ViewProviderPart)

ViewProviderBody::ViewProviderBody()
{
    pcBodyChildren = new SoSeparator();
    pcBodyChildren->ref();
    pcBodyTip = new SoSeparator();
    pcBodyTip->ref();

    sPixmap = "PartDesign_Body_Tree.svg";
}

ViewProviderBody::~ViewProviderBody()
{
    pcBodyChildren->unref ();
    pcBodyTip->unref ();
}

void ViewProviderBody::attach(App::DocumentObject *pcFeat)
{
    // call parent attach method
    ViewProviderPart::attach(pcFeat);
    PartDesign::Body *body = static_cast <PartDesign::Body *> (pcFeat);

    App::DocumentObject *tip = body->Tip.getValue ();

    if (tip) {
        Gui::ViewProvider *vp = Gui::Application::Instance->getViewProvider (tip);
        if (vp) {
            pcBodyTip->addChild ( vp->getRoot () );
        }
    }

    addDisplayMaskMode(pcBodyChildren, "Through");
    addDisplayMaskMode(pcBodyTip, "Tip");

    App::Document *adoc  = pcObject->getDocument ();
    Gui::Document *gdoc = Gui::Application::Instance->getDocument ( adoc ) ;

    assert ( adoc );
    assert ( gdoc );

    connectChangedObjectApp = adoc->signalChangedObject.connect (
            boost::bind ( &ViewProviderBody::slotChangedObjectApp, this, _1, _2) );

    connectChangedObjectGui = gdoc->signalChangedObject.connect (
            boost::bind ( &ViewProviderBody::slotChangedObjectGui, this, _1, _2) );
}

// TODO on activating the body switch to the "Through" mode (2015-09-05, Fat-Zer)
// TODO differnt icon in tree if mode is Through (2015-09-05, Fat-Zer)
// TODO drag&drop (2015-09-05, Fat-Zer)
// TODO Add activate () call (2015-09-08, Fat-Zer)

void ViewProviderBody::setDisplayMode(const char* ModeName)
{
    if ( strcmp("Through",ModeName)==0 )
        setDisplayMaskMode("Through");
    // TODO Use other Part::features display modes instead of the "Tip" (2015-09-08, Fat-Zer)
    if ( strcmp("Tip",ModeName)==0 )
        setDisplayMaskMode("Tip");
    // TODO When switching into Tip mode switch it's visability to true (2015-09-05, Fat-Zer)
    ViewProviderGeometryObject::setDisplayMode( ModeName );
}

std::vector<std::string> ViewProviderBody::getDisplayModes(void) const {
    return {"Through" , "Tip"};
}


bool ViewProviderBody::doubleClicked(void)
{
    // assure the PartDesign workbench
    Gui::Command::assureWorkbench("PartDesignWorkbench");

    // and set correct active objects
    auto* part = App::Part::getPartOfObject ( getObject() );
    if ( part && part != getActiveView()->getActiveObject<App::Part*> ( PARTKEY ) ) {
        Gui::Command::doCommand ( Gui::Command::Gui,
                "Gui.activeView().setActiveObject('%s', App.activeDocument().%s)",
                PARTKEY, part->getNameInDocument() );
    }

    Gui::Command::doCommand ( Gui::Command::Gui,
            "Gui.activeView().setActiveObject('%s', App.activeDocument().%s)",
            PDBODYKEY, this->getObject()->getNameInDocument() );

    return true;
}

std::vector<App::DocumentObject*> ViewProviderBody::claimChildren(void)const
{
    PartDesign::Body* body= static_cast<PartDesign::Body*> ( getObject () );
    const std::vector<App::DocumentObject*> &model = body->Model.getValues ();
    std::set<App::DocumentObject*> outSet; //< set of objects not to claim (childrens of childrens)

    // search for objects handled (claimed) by the features
    for( auto obj: model){
        if (!obj) { continue; }
        Gui::ViewProvider* vp = Gui::Application::Instance->getViewProvider ( obj );
        if (!vp) { continue; }

        auto children = vp->claimChildren();
        std::remove_copy ( children.begin (), children.end (), std::inserter (outSet, outSet.begin () ), nullptr);
    }

    // remove the otherwise handled objects, preserving their order so the order in the TreeWidget is correct
    std::vector<App::DocumentObject*> Result;

    if (body->Origin.getValue()) { // Clame for the Origin
        Result.push_back (body->Origin.getValue());
    }
    if (body->BaseFeature.getValue()) { // Clame for the base feature
        Result.push_back (body->BaseFeature.getValue());
    }

    // claim for rest content not claimed by any other features
    std::remove_copy_if (model.begin(), model.end(), std::back_inserter (Result),
            [outSet] (App::DocumentObject* obj) {
                return outSet.find (obj) != outSet.end();
            } );

    return Result;
}


std::vector<App::DocumentObject*> ViewProviderBody::claimChildren3D(void)const
{
    PartDesign::Body* body = static_cast<PartDesign::Body*>(getObject());

    const std::vector<App::DocumentObject*> & features = body->Model.getValues();
    App::DocumentObject *originObj   = body->Origin.getValue();
    App::DocumentObject *baseFeature = body->BaseFeature.getValue();

    std::vector<App::DocumentObject*> rv;

    if ( body->Origin.getValue() ) { // Add origin
        rv.push_back (body->Origin.getValue());
    }
    if ( body->BaseFeature.getValue() ) { // Add Base Feature
        rv.push_back (body->BaseFeature.getValue());
    }

    // Add all other stuff
    std::copy (features.begin(), features.end(), std::back_inserter (rv) );

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

bool ViewProviderBody::onDelete ( const std::vector<std::string> &) {
    // TODO May be do it conditionally? (2015-09-05, Fat-Zer)
    Gui::Command::doCommand(Gui::Command::Doc,
            "App.getDocument(\"%s\").getObject(\"%s\").removeModelFromDocument()"
            ,getObject()->getDocument()->getName(), getObject()->getNameInDocument());
    return true;
}

void ViewProviderBody::updateData(const App::Property* prop)
{
    PartDesign::Body* body = static_cast<PartDesign::Body*>(getObject());

    if (prop == &body->Model || prop == &body->BaseFeature) {
        // update sizes of origins and datums
        updateOriginDatumSize ();
    } else if (prop == &body->Tip) {
        // Adjust the internals to display
        App::DocumentObject *tip = body->Tip.getValue ();

        if (tip) {
            Gui::ViewProvider *vp = Gui::Application::Instance->getViewProvider (tip);
            if (vp) {
                SoNode *tipRoot = vp->getRoot ();
                if ( pcBodyTip->findChild ( tipRoot ) == -1 ) {
                    pcBodyTip->removeAllChildren ();
                    pcBodyTip->addChild ( tipRoot );
                }
                // Else our tip is already shown
            } else {
                pcBodyTip->removeAllChildren ();
            }
        } else {
            pcBodyTip->removeAllChildren ();
        }
    }

    PartGui::ViewProviderPart::updateData(prop);
}


void ViewProviderBody::slotChangedObjectApp ( const App::DocumentObject& obj, const App::Property& prop ) {
    if (!obj.isDerivedFrom ( Part::Feature::getClassTypeId () ) ) { // we are intrested only in Part::Features
        return;
    }

    const Part::Feature *feat = static_cast <const Part::Feature *>(&obj);

    if ( &feat->Shape != &prop && &feat->Placement != &prop) { // react only on changes in shapes and placement
        return;
    }

    PartDesign::Body *body = static_cast<PartDesign::Body*> ( getObject() );
    if ( body && body->hasFeature (&obj ) ) {
        updateOriginDatumSize ();
    }
}

void ViewProviderBody::slotChangedObjectGui (
        const Gui::ViewProviderDocumentObject& vp, const App::Property& prop )
{
    if (&vp.Visibility != &prop) { // react only on visability changes
        return;
    }

    if ( !vp.isDerivedFrom ( Gui::ViewProviderOrigin::getClassTypeId () ) &&
         !vp.isDerivedFrom ( Gui::ViewProviderOriginFeature::getClassTypeId () ) ) {
        // Ignore origins to avoid infinite recursion (not likely in a well-formed document,
        //          but may happen in documents designed in old versions of assembly branch )
        return;
    }

    PartDesign::Body *body = static_cast<PartDesign::Body*> ( getObject() );
    App::DocumentObject *obj = vp.getObject ();

    if ( body && obj && body->hasFeature ( obj ) ) {
        updateOriginDatumSize ();
    }
}

void ViewProviderBody::updateOriginDatumSize () {
    PartDesign::Body *body = static_cast<PartDesign::Body *> ( getObject() );
    // Use different bounding boxes for datums and for origins:
    Gui::View3DInventorViewer* viewer = static_cast<Gui::View3DInventor*>(this->getActiveView())->getViewer();
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
