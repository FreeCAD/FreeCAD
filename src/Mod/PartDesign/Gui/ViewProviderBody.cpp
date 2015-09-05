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
# include <algorithm>
# include <Inventor/nodes/SoGroup.h>
# include <Inventor/nodes/SoSeparator.h>
#endif

#include <Base/Console.h>
#include <App/Part.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/Application.h>

#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/FeatureSketchBased.h>
#include <Mod/PartDesign/App/FeatureMultiTransform.h>
#include <Mod/PartDesign/App/DatumLine.h>
#include <Mod/PartDesign/App/DatumPlane.h>
#include <Mod/PartDesign/App/DatumCS.h>

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
}

// TODO on activating the body switch to the "Through" mode (2015-09-05, Fat-Zer)
// TODO differnt icon in tree if mode is Through (2015-09-05, Fat-Zer)
// TODO drag&drop (2015-09-05, Fat-Zer)

void ViewProviderBody::setDisplayMode(const char* ModeName)
{
    if ( strcmp("Through",ModeName)==0 )
        setDisplayMaskMode("Through");
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

    //and set correct active objects
    auto* part = PartDesignGui::getPartFor(getObject(), false);
    if(part!=Gui::Application::Instance->activeView()->getActiveObject<App::Part*>(PARTKEY))
        Gui::Command::doCommand(Gui::Command::Gui, "Gui.activeView().setActiveObject('%s', App.activeDocument().%s)", PARTKEY, part->getNameInDocument());

    Gui::Command::doCommand(Gui::Command::Gui, "Gui.activeView().setActiveObject('%s', App.activeDocument().%s)", PDBODYKEY, this->getObject()->getNameInDocument());

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

    if (body->Origin.getValue()) { // Add origin
        rv.push_back (body->Origin.getValue());
    }
    if ( body->BaseFeature.getValue() ) { // Add Base Feature
        rv.push_back (body->BaseFeature.getValue());
    }

    std::copy (features.begin(), features.end(), std::back_inserter (rv) );
    // TODO Check what will happen if BaseFature will be shared by severral bodies (2015-08-04, Fat-Zer)
    return rv;
}

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
        // TODO Write this (2015-09-05, Fat-Zer)
    } else if (prop == &body->Tip) {
        // Adjust the internals to display
        App::DocumentObject *tip = body->Tip.getValue ();

        if (tip) {
            Gui::ViewProvider *vp = Gui::Application::Instance->getViewProvider (tip);
            if (vp) {
                SoNode *tipRoot = vp->getRoot ();
                if ( pcBodyTip->findChild (tipRoot) == -1 ) {
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

    // TODO rewrite this, it's quite a hacky way of notifying (2015-09-05, Fat-Zer)
    std::vector<App::DocumentObject*> features = body->Model.getValues();
    for (std::vector<App::DocumentObject*>::const_iterator f = features.begin(); f != features.end(); f++) {
        App::PropertyPlacement* plm = NULL;
        if ((*f)->getTypeId().isDerivedFrom(PartDesign::Line::getClassTypeId()))
            plm = &(static_cast<PartDesign::Line*>(*f)->Placement);
        else if ((*f)->getTypeId().isDerivedFrom(PartDesign::Plane::getClassTypeId()))
            plm = &(static_cast<PartDesign::Plane*>(*f)->Placement);
        else if ((*f)->getTypeId().isDerivedFrom(PartDesign::CoordinateSystem::getClassTypeId()))
            plm = &(static_cast<PartDesign::CoordinateSystem*>(*f)->Placement);

        if (plm != NULL) {
            Gui::ViewProviderDocumentObject* vp = dynamic_cast<Gui::ViewProviderDocumentObject*>(Gui::Application::Instance->getViewProvider(*f));
            if (vp != NULL)
                vp->updateData(plm);
        }
    }

    PartGui::ViewProviderPart::updateData(prop);
}
