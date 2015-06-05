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
# include <Inventor/nodes/SoGroup.h>
#endif

#include "ViewProviderBody.h"
#include "Workbench.h"
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/Application.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/FeatureSketchBased.h>
#include <Mod/PartDesign/App/FeatureMultiTransform.h>
#include <Mod/PartDesign/App/DatumLine.h>
#include <Mod/PartDesign/App/DatumPlane.h>
#include <Mod/PartDesign/App/DatumCS.h>
#include <algorithm>

#include "Base/Console.h"
#include <App/Part.h>

using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderBody,PartGui::ViewProviderPart)

ViewProviderBody::ViewProviderBody()
{
    pcBodyChildren = new SoGroup();
    pcBodyChildren->ref();
    pcBodyTip     = new SoGroup();
    pcBodyTip->ref();
    sPixmap = "PartDesign_Body_Tree.svg";
}

ViewProviderBody::~ViewProviderBody()
{
    pcBodyChildren->unref();
    pcBodyChildren = 0;
    pcBodyTip->unref();
    pcBodyTip = 0;
}


void ViewProviderBody::attach(App::DocumentObject *pcFeat)
{
    // call parent attach method
    ViewProviderPart::attach(pcFeat);


    // putting all together with the switch
    addDisplayMaskMode(pcBodyTip, "Body");
    addDisplayMaskMode(pcBodyChildren, "Through");
}

void ViewProviderBody::setDisplayMode(const char* ModeName)
{
    if ( strcmp("Body",ModeName)==0 )
        setDisplayMaskMode("Body");
    if ( strcmp("Main",ModeName)==0 )
        setDisplayMaskMode("Main");
    if ( strcmp("Through",ModeName)==0 )
        setDisplayMaskMode("Through");

    ViewProviderGeometryObject::setDisplayMode( ModeName );
}

std::vector<std::string> ViewProviderBody::getDisplayModes(void) const
{
    // get the modes of the father
    std::vector<std::string> StrList = ViewProviderGeometryObject::getDisplayModes();

    // add your own modes
    StrList.push_back("Through");
    StrList.push_back("Body");

    return StrList;
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
    std::vector<App::DocumentObject*> Model = static_cast<PartDesign::Body*>(getObject())->Model.getValues();
    std::set<App::DocumentObject*> OutSet;

    // search for objects handled (claimed) by the features
    for(std::vector<App::DocumentObject*>::const_iterator it = Model.begin();it!=Model.end();++it){
        if (*it == NULL) continue;
        Gui::ViewProvider* vp = Gui::Application::Instance->getViewProvider(*it);
        if (vp == NULL) continue;
        std::vector<App::DocumentObject*> children = vp->claimChildren();
        for (std::vector<App::DocumentObject*>::const_iterator ch = children.begin(); ch != children.end(); ch++) {
            if ((*ch) != NULL)
                OutSet.insert(*ch);
        }
    }

    // remove the otherwise handled objects, preserving their order so the order in the TreeWidget is correct
    std::vector<App::DocumentObject*> Result;
    for (std::vector<App::DocumentObject*>::const_iterator it = Model.begin();it!=Model.end();++it) {
        if (OutSet.find(*it) == OutSet.end())
            Result.push_back(*it);
    }

    // return the rest as claim set of the Body
    return Result;
}


std::vector<App::DocumentObject*> ViewProviderBody::claimChildren3D(void)const
{
    //std::vector<App::DocumentObject*> children = static_cast<PartDesign::Body*>(getObject())->Model.getValues();
    //Base::Console().Error("Body 3D claimed children:\n");
    //for (std::vector<App::DocumentObject*>::const_iterator o = children.begin(); o != children.end(); o++)
    //    Base::Console().Error("%s\n", (*o)->getNameInDocument());
    return static_cast<PartDesign::Body*>(getObject())->Model.getValues();

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

void ViewProviderBody::updateData(const App::Property* prop)
{
    //Base::Console().Error("ViewProviderBody::updateData for %s\n", getObject()->getNameInDocument());
    //if (ActiveGuiDoc == NULL)
    //    // PartDesign workbench not active
    //    return PartGui::ViewProviderPart::updateData(prop);

    //if ((/*prop->getTypeId() == App::PropertyBool::getClassTypeId() && strcmp(prop->getName(),"IsActive") == 0) ||*/
    //    (prop->getTypeId() == App::PropertyLink::getClassTypeId() && strcmp(prop->getName(),"Tip") == 0) ||
    //    (prop->getTypeId() == App::PropertyLinkList::getClassTypeId() && strcmp(prop->getName(),"Model") == 0))
    //   // updateTree();

    // Update the visual size of datums
    PartDesign::Body* body = static_cast<PartDesign::Body*>(getObject());
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
