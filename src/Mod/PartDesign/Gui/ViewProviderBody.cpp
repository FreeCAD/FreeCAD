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
#include <Gui/Command.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/FeatureSketchBased.h>
#include <algorithm>

using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderBody,PartGui::ViewProviderPart)

ViewProviderBody::ViewProviderBody()
{
    pcBodyChildren = new SoGroup();
    pcBodyChildren->ref();
    pcBodyTip     = new SoGroup();
    pcBodyTip->ref();
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
    Gui::Command::addModule(Gui::Command::Gui,"PartDesignGui");
    Gui::Command::doCommand(Gui::Command::Doc,"PartDesignGui.setActivePart(App.activeDocument().%s)",this->getObject()->getNameInDocument());
    return true;
}

std::vector<App::DocumentObject*> ViewProviderBody::claimChildren(void)const
{
    std::vector<App::DocumentObject*> Model = static_cast<PartDesign::Body*>(getObject())->Model.getValues();
    std::set<App::DocumentObject*> OutSet;

    // search for objects handled (claimed) by the features
    for(std::vector<App::DocumentObject*>::const_iterator it = Model.begin();it!=Model.end();++it){
        // sketches of SketchBased features get claimed under the feature so has to be removed from the Body
        if ((*it)->isDerivedFrom(PartDesign::SketchBased::getClassTypeId())){
            OutSet.insert(static_cast<PartDesign::SketchBased*>(*it)->Sketch.getValue());
        }
    }

    // remove the otherwise handled objects 
    std::vector<App::DocumentObject*> Result(Model.size());
    sort (Model.begin(), Model.end());   
    std::vector<App::DocumentObject*>::iterator it = set_difference (Model.begin(), Model.end(), OutSet.begin(),OutSet.end(), Result.begin());

    // return the rest as claim set of the Body
    return std::vector<App::DocumentObject*>(Result.begin(),it);
}


std::vector<App::DocumentObject*> ViewProviderBody::claimChildren3D(void)const
{

    return static_cast<PartDesign::Body*>(getObject())->Model.getValues();

}
