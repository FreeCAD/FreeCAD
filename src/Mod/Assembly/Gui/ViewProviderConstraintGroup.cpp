/***************************************************************************
 *   Copyright (c) 2012 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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

#include "ViewProviderConstraintGroup.h"
#include <Gui/Command.h>
#include <Gui/Document.h>

#include <Mod/Assembly/App/ConstraintGroup.h>

using namespace AssemblyGui;

PROPERTY_SOURCE(AssemblyGui::ViewProviderConstraintGroup,Gui::ViewProviderDocumentObject)

ViewProviderConstraintGroup::ViewProviderConstraintGroup()
{
  sPixmap = "Assembly_Assembly_Constraints_Tree.svg";
}

ViewProviderConstraintGroup::~ViewProviderConstraintGroup()
{
}

bool ViewProviderConstraintGroup::doubleClicked(void)
{
    return false;
}

void ViewProviderConstraintGroup::attach(App::DocumentObject *pcFeat)
{
    // call parent attach method
    ViewProviderDocumentObject::attach(pcFeat);


    // putting all together with the switch
    //addDisplayMaskMode(getChildRoot(), "Main");
}

void ViewProviderConstraintGroup::setDisplayMode(const char* ModeName)
{
    if ( strcmp("Main",ModeName)==0 )
        setDisplayMaskMode("Main");

    ViewProviderDocumentObject::setDisplayMode( ModeName );
}

std::vector<std::string> ViewProviderConstraintGroup::getDisplayModes(void) const
{
    // get the modes of the father
    std::vector<std::string> StrList = ViewProviderDocumentObject::getDisplayModes();

    // add your own modes
    StrList.push_back("Main");

    return StrList;
}


std::vector<App::DocumentObject*> ViewProviderConstraintGroup::claimChildren(void)const
{
    std::vector<App::DocumentObject*> temp(static_cast<Assembly::ConstraintGroup*>(getObject())->Constraints.getValues());

    return temp;
}

std::vector<App::DocumentObject*> ViewProviderConstraintGroup::claimChildren3D(void)const
{

    //return static_cast<Assembly::ConstraintGroup*>(getObject())->Constraints.getValues();
    return std::vector<App::DocumentObject*> ();
}
